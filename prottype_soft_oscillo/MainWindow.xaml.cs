using System;
using System.Windows;

namespace RefreshDemo
{
    using System.ComponentModel;
    using System.Threading;

    using OxyPlot;
    using OxyPlot.Axes;
    using OxyPlot.Series;
    using System.Linq;
    using System.Windows.Controls;

    // DllImportに必要
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using System.Windows.Threading;
    using System.IO.Ports;
    using System.Windows.Input;
    using System.Diagnostics;

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //WPFアプリはAnyCPUでビルドすると実行ファイル内に32bitと64bit共存するようになります。
        //DLLにはそのような機能はありませんので
        //32bitまたは64bitかを整理してDllを動作環境下に配置しないと動作しません。

        //---------------------------------------------------------------------
        //MotionSensorFunc.dll
        //---------------------------------------------------------------------
        [DllImport("MotionSensorFunc.dll")]
        private extern static bool Initialize();

        [DllImport("MotionSensorFunc.dll")]
        private extern static void Finilize();

        [DllImport("MotionSensorFunc.dll")]
        private extern static bool Start(string SpindleComName, string MotionComName);

        [DllImport("MotionSensorFunc.dll")]
        private extern static void Stop();

        [DllImport("MotionSensorFunc.dll")]
        private extern static int GetData(IntPtr Data, int DataSize);

        [DllImport("MotionSensorFunc.dll")]
        private extern static int GetError();

        //---------------------------------------------------------------------
        //NotifyDeviceFunc.dll
        //---------------------------------------------------------------------
        [DllImport("NotifyDeviceFunc.dll")]
        private extern static bool Start(string PortName);

        [DllImport("NotifyDeviceFunc.dll")]
        private extern static void StopNotify();

        [DllImport("NotifyDeviceFunc.dll")]
        private extern static void Notify(int time);

        public PlotModel PlotModel { get; set; }
        const int MAX_DATA_COUNT = 4;
        const int X_DISPLAY_RANGE = 4;
        private bool[] is_visible = Enumerable.Repeat<bool>(true, MAX_DATA_COUNT).ToArray();
        private double[] offset = new double[MAX_DATA_COUNT];
        private double[] scale = new double[MAX_DATA_COUNT];
        private bool is_device_start = false;
        private bool is_stop_resrve = true;
        private int tickcount = 0;
        private bool is_stop = true;
        private int THREAD_SLEEP_TIME = 100;
        const double k = X_DISPLAY_RANGE;
        const int DATA_BUF_SIZE =(int) (k / 0.01);
        private double[,] data_buf = new double[8,DATA_BUF_SIZE];
        private string primado_name="";
        private string tsdn_name = "";
        private bool start_drill_monitor = false;

        //---------------------------------------------------------------------
        //シリアルポートの列挙は以下で可能
        //---------------------------------------------------------------------
        string[] PortList = SerialPort.GetPortNames();

        public MainWindow()
        {
            InitializeComponent();

            //データインデックスを0に予め設定する
            this.DataIndex.SelectedIndex = 0;

            //オフセット値初期化
            InitializeControl();

            //DLL　初期化
            Initialize();

            //Demo (sin: -1.0 -> 1.0)
            this.PlotModel = CreatePlotModel(-5, 5);

            DataContext = this;
            var worker = new BackgroundWorker { WorkerSupportsCancellation = true };
            double x = 0;
            double stop_x = 0;

            worker.DoWork += (s, e) =>
            {
                //DLLからのデータ取得領域確保
                double[] ary = new double[8];
　
                //バックグラウンドワーカーを作成する（作業スレッドを作成すると同じ）
                //ここでセンサーから値を取得する？
                while (!worker.CancellationPending)
                {
                    if (is_stop_resrve) {
                        int count = Environment.TickCount - tickcount;
                        if (count < THREAD_SLEEP_TIME) {
                             stop_x = x;
                        }
                        if (count > 1000) {
                            is_stop = true;
                        }
                    };
                    if (is_stop) {
                        //100ミリ秒休止
                        Thread.Sleep(THREAD_SLEEP_TIME);
                        continue;
                    }

                    lock (this.PlotModel.SyncRoot)
                    {
                        //データ取得処理
                        GetDataProc(ref ary);

                        this.PlotModel.Title = "Plot updated: " + DateTime.Now;
                        for (int i = 0; i < MAX_DATA_COUNT; ++i) {

                            //表示データを操作します。
                            Func<double, double> convert;
                            if (i == 3) {
                                convert = delegate (double a)
                                {
                                    if(is_stop_resrve ){
                                        return (stop_x + X_DISPLAY_RANGE) < a ? 1 : 0;    
                                    }else{ 
                                        return 0;
                                    }
                                };
                            } else {
                                convert = delegate (double a)
                                {
                                    
                                    double r = Math.Sin(a);
                                    
                                    //データをバッファリングしないとデータが表示されませんので
                                    //ため込んだら表示するような処理が必要です。
/*
                                    double l =ary[i];
                                    int divide = (int)((double)(X_DISPLAY_RANGE) / 0.01);
                                    int index = ((int)(a / 0.01)) % divide; 
                                    if (data_buf[index] > 0){
                                        r =  data_buf[i,index];
                                    }else{
                                        r = 0;
                                    }
*/
                                    return r * scale[i] + offset[i];
                                };
                            }

                            //sin関数を使ってオシロスコープ感を出す
                            // (0.01 + i * 0.2) -> 変化を目立たせるため。
                            this.PlotModel.Series[i] = new FunctionSeries(convert, x, x + X_DISPLAY_RANGE,0.01);
                            this.PlotModel.Series[i].IsVisible = is_visible[i];
                            this.PlotModel.Series[i].TextColor=OxyColor.FromArgb(0,0,0,0);
                        }
                    }
                    x += 0.1;
                    PlotModel.InvalidatePlot(true);
                    //100ミリ秒休止
                    Thread.Sleep(THREAD_SLEEP_TIME);
                }
            };
            worker.RunWorkerAsync();
            this.Closed += (s, e) => {
                worker.CancelAsync();
                //DLL　動作終了
                Stop();

                //DLL　終了
                Finilize();
            };
        }

        private static PlotModel CreatePlotModel(double min, double max)
        {
            var model = new PlotModel();
            //垂直軸を固定するため、最小値、最大値を指定する
            var verticalAxis = new LinearAxis { Position = AxisPosition.Left, Minimum = min, Maximum = max };
            model.Axes.Add(verticalAxis);
            model.Axes.Add(new LinearAxis { Position = AxisPosition.Bottom });

            //表示したい折れ線グラフ数分、領域を確保する（今回は4）
            for (int i = 0; i < MAX_DATA_COUNT; ++i) {
                model.Series.Add(new FunctionSeries());
            }
            return model;
        }

        private void CheckBox_Changed(object sender, RoutedEventArgs e)
        {
            //var index = get_checkbox_index(sender);
            //チェックボックスで表示可否を操作する
            is_visible[DataIndex.SelectedIndex] = (sender as CheckBox)?.IsChecked ?? false;
        }

        private int get_checkbox_index(object sender)
        {
            //チェックボックスの名前からインデックスを取得する
            var ck = (sender as CheckBox);
            String number = ck.Name;
            return int.Parse(number.Substring(number.Length - 1, 1));
        }

        private  void  GetDataProc(ref double[] ary)
        {
            int len = ary.Length;

            //DLL側がアクセスできる領域に確保する
            GCHandle gcH = GCHandle.Alloc(ary, GCHandleType.Pinned);

            //DLL　データ取得
            int re = GetData(gcH.AddrOfPinnedObject(),len);
            /*
            if (re > 0) {
                GetError();
            }
            */

            gcH.Free();
        }

        private void InitializeControl()
        {
            for (int i = 0; i < MAX_DATA_COUNT; ++i){
                offset[i] = 0.0;
                scale[i] = 1.1;
            }
            foreach (var a in PortList) {
                this.NotifyDeviceName.Items.Add(a);
                this.PrimadoPortName.Items.Add(a);
                this.TSNDPortName.Items.Add(a);
            }

            for (int i = 0; i < 8; ++i)
            {
                for (int j = 0; j < DATA_BUF_SIZE; ++j)
                {
                    this.data_buf[i,j] = -1;
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void SetProperty<T>(ref T field, T value, [CallerMemberName]string propertyName = null)
        {
            field = value;
            var h = this.PropertyChanged;
            if (h != null) { h(this, new PropertyChangedEventArgs(propertyName)); }
        }

        public double Scale
        {
            get { return scale[DataIndex.SelectedIndex]; }
            set { this.SetProperty(ref scale[DataIndex.SelectedIndex], value); }
        }

        public double Offset
        {
            get { return offset[DataIndex.SelectedIndex]; }
            set { this.SetProperty(ref offset[DataIndex.SelectedIndex], value); }
        }

         public bool Visible
        {
            get { return is_visible[DataIndex.SelectedIndex]; }
            set { this.SetProperty(ref is_visible[DataIndex.SelectedIndex], value); }
        }

        private void DataIndex_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            this.OffsetTextBox.Text = offset[DataIndex.SelectedIndex].ToString();
            this.ScaleTextBox.Text = scale[DataIndex.SelectedIndex].ToString();
            this.ShowDataCheck.IsChecked = is_visible[DataIndex.SelectedIndex];
        }

        private void NotifyDeviceName_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            //Notify test
            is_device_start = Start(NotifyDeviceName.SelectedItem.ToString());
        }
        
        private void Window_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            //Spaceキーが押されたらデバイスに通知をします。
            if (e.Key == Key.Space) {
                stop_core();
            }
        }

        private void stop_core()
        {
            //ドリルモニターがスタート済みでないと解除しない
            if( !start_drill_monitor )return;

            //
            is_stop_resrve = !is_stop_resrve;
            if (is_stop_resrve) {
                tickcount = Environment.TickCount;
                //デバイスのポートがOpenしていたら通知処理を実行します。
                if (is_device_start){
                    Notify(1000);
                }
            } else {
                tickcount = 0;
                is_stop = false;
            }
        }

        private void PrimadoPortName_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            primado_name = this.PrimadoPortName.SelectedItem.ToString();
        }

        private void TSNDPortName_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            tsdn_name = this.TSNDPortName.SelectedItem.ToString();
        }

        private void OscilloStart_Click(object sender, RoutedEventArgs e)
        {
            if(primado_name.Length > 0 && tsdn_name.Length > 0){
                //DLL　動作開始
                start_drill_monitor = Start(primado_name, tsdn_name);
                stop_core();
            }
        }
    }
}