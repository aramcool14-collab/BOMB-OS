using System.Drawing.Imaging;
using System.IO.Ports;

namespace BombViewer;

internal static class Program
{
    [STAThread]
    static void Main()
    {
        ApplicationConfiguration.Initialize();
        Application.Run(new ViewerForm());
    }
}

internal sealed class ViewerForm : Form
{
    private readonly ComboBox ports = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 150 };
    private readonly Button refresh = new() { Text = "Refresh", AutoSize = true };
    private readonly Button connect = new() { Text = "Connect", AutoSize = true };
    private readonly Label status = new() { Text = "Not connected", AutoSize = true, Padding = new Padding(8) };
    private readonly PictureBox screen = new()
    {
        Width = 640, Height = 480, BackColor = Color.Black,
        SizeMode = PictureBoxSizeMode.Zoom
    };
    private SerialPort? serial;
    private CancellationTokenSource? readerStop;

    public ViewerForm()
    {
        Text = "BOMB OS Screen Viewer";
        ClientSize = new Size(700, 550);
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;

        var bar = new FlowLayoutPanel { Dock = DockStyle.Top, Height = 42, Padding = new Padding(8) };
        bar.Controls.Add(new Label { Text = "USB COM port:", AutoSize = true, Padding = new Padding(0, 7, 0, 0) });
        bar.Controls.Add(ports);
        bar.Controls.Add(refresh);
        bar.Controls.Add(connect);
        bar.Controls.Add(status);
        screen.Location = new Point(30, 55);
        Controls.Add(screen);
        Controls.Add(bar);

        refresh.Click += (_, _) => RefreshPorts();
        connect.Click += async (_, _) => await ToggleConnection();
        FormClosing += (_, _) => StopReader();
        RefreshPorts();
    }

    private void RefreshPorts()
    {
        string? previous = ports.SelectedItem as string;
        ports.Items.Clear();
        foreach (string name in SerialPort.GetPortNames().OrderBy(x => x))
            ports.Items.Add(name);
        if (previous != null && ports.Items.Contains(previous)) ports.SelectedItem = previous;
        else if (ports.Items.Count > 0) ports.SelectedIndex = 0;
        status.Text = ports.Items.Count == 0 ? "No COM ports found" : "Choose the Pi's USB COM port";
    }

    private async Task ToggleConnection()
    {
        if (serial?.IsOpen == true)
        {
            StopReader();
            connect.Text = "Connect";
            status.Text = "Disconnected";
            return;
        }

        if (ports.SelectedItem is not string portName)
        {
            status.Text = "No COM port selected";
            return;
        }

        try
        {
            serial = new SerialPort(portName, 115200, Parity.None, 8, StopBits.One)
            {
                ReadTimeout = 500,
                WriteTimeout = 1000
            };
            serial.Open();
            readerStop = new CancellationTokenSource();
            connect.Text = "Disconnect";
            status.Text = $"Listening on {portName} at 115200 baud";
            CancellationToken token = readerStop.Token;
            _ = Task.Run(() => ReadFrames(token)).ContinueWith(t =>
            {
                if (IsDisposed) return;
                BeginInvoke(new Action(() =>
                {
                    if (serial?.IsOpen != true)
                    {
                        connect.Text = "Connect";
                        if (t.IsFaulted) status.Text = "Reader error: " + t.Exception!.GetBaseException().Message;
                    }
                }));
            }, TaskScheduler.Default);
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            status.Text = "Error: " + ex.Message;
            StopReader();
            connect.Text = "Connect";
        }
    }

    private void StopReader()
    {
        readerStop?.Cancel();
        readerStop = null;
        try { if (serial?.IsOpen == true) serial.Close(); } catch { }
        serial?.Dispose();
        serial = null;
    }

    private void ReadFrames(CancellationToken token)
    {
        var input = new List<byte>(160000);
        byte[] chunk = new byte[4096];

        while (!token.IsCancellationRequested && serial?.IsOpen == true)
        {
            try
            {
                int count = serial.Read(chunk, 0, chunk.Length);
                for (int i = 0; i < count; i++) input.Add(chunk[i]);

                while (TryTakeFrame(input, out int width, out int height, out byte[]? pixels))
                {
                    Bitmap bitmap = MakeBitmap(width, height, pixels!);
                    BeginInvoke(new Action(() =>
                    {
                        Image? old = screen.Image;
                        screen.Image = bitmap;
                        old?.Dispose();
                        status.Text = $"{width} x {height} RGB565";
                    }));
                }

                if (input.Count > 200000) input.RemoveRange(0, input.Count - 16);
            }
            catch (TimeoutException) { }
            catch (InvalidOperationException) { break; }
            catch (IOException) { break; }
        }
    }

    private static bool TryTakeFrame(List<byte> data, out int width, out int height, out byte[]? pixels)
    {
        width = height = 0;
        pixels = null;

        int magic = -1;
        for (int i = 0; i <= data.Count - 4; i++)
        {
            if (data[i] == (byte)'B' && data[i + 1] == (byte)'O' &&
                data[i + 2] == (byte)'M' && data[i + 3] == (byte)'B')
            {
                magic = i;
                break;
            }
        }

        if (magic < 0)
        {
            if (data.Count > 3) data.RemoveRange(0, data.Count - 3);
            return false;
        }
        if (magic > 0) data.RemoveRange(0, magic);
        if (data.Count < 12) return false;

        width = data[4] | (data[5] << 8);
        height = data[6] | (data[7] << 8);
        uint length = (uint)(data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24));
        if (width < 1 || width > 640 || height < 1 || height > 480 || length != (uint)(width * height * 2))
        {
            data.RemoveAt(0);
            return false;
        }

        if (data.Count < 12 + (int)length) return false;
        pixels = data.GetRange(12, (int)length).ToArray();
        data.RemoveRange(0, 12 + (int)length);
        return true;
    }

    private static Bitmap MakeBitmap(int width, int height, byte[] rgb565)
    {
        var bitmap = new Bitmap(width, height, PixelFormat.Format24bppRgb);
        var rect = new Rectangle(0, 0, width, height);
        BitmapData locked = bitmap.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format24bppRgb);
        try
        {
            int stride = locked.Stride;
            byte[] row = new byte[stride];
            for (int y = 0; y < height; y++)
            {
                Array.Clear(row);
                for (int x = 0; x < width; x++)
                {
                    int p = (y * width + x) * 2;
                    ushort value = (ushort)(rgb565[p] | (rgb565[p + 1] << 8));
                    int r = ((value >> 11) & 0x1F) * 255 / 31;
                    int g = ((value >> 5) & 0x3F) * 255 / 63;
                    int b = (value & 0x1F) * 255 / 31;
                    int q = x * 3;
                    row[q] = (byte)b;
                    row[q + 1] = (byte)g;
                    row[q + 2] = (byte)r;
                }
                System.Runtime.InteropServices.Marshal.Copy(row, 0, locked.Scan0 + y * stride, stride);
            }
        }
        finally { bitmap.UnlockBits(locked); }
        return bitmap;
    }
}
