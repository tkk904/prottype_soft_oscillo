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

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //WPFアプリはAnyCPUでビルドすると実行ファイル内に32bitと64bit共存するようになります。
        //DLLにはそのような機能はありませんので
        //32bitまたは64bitかを整理してDllを動作環境下に配置しないと動作しません。
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

        public PlotModel PlotModel { get; set; }
        const int MAX_DATA_COUNT = 9;
        private bool[] is_visible = Enumerable.Repeat<bool>(true, MAX_DATA_COUNT).ToArray();
        private double[] offset = new double[MAX_DATA_COUNT];
        private double[] scale = new double[MAX_DATA_COUNT];

        public MainWindow()
        {
            InitializeComponent();

            this.DataIndex.SelectedIndex = 0;

            //オフセット値初期化
            InitializeOffset();

            //DLL　初期化
            Initialize();
            
            //DLL　動作開始
            string hoge = "COM1";
            Start(hoge, "COM2");

            //Demo (sin: -1.0 -> 1.0)
            this.PlotModel = CreatePlotModel(-5, 5);

            DataContext = this;
            var worker = new BackgroundWorker { WorkerSupportsCancellation = true };
            double x = 0;

            worker.DoWork += (s, e) =>
            {
                //DLLからのデータ取得領域確保
                double[] ary = new double[8];
　
                //バックグラウンドワーカーを作成する（作業スレッドを作成すると同じ）
                //ここでセンサーから値を取得する？
                while (!worker.CancellationPending)
                {
                    lock (this.PlotModel.SyncRoot)
                    {
                        //データ取得処理
                        GetDataProc(ref ary);

                        this.PlotModel.Title = "Plot updated: " + DateTime.Now;
                        for (int i = 0; i < MAX_DATA_COUNT; ++i) {

                            Func<double, double> convert = delegate (double a)
                            {
                                double r = Math.Sin(a);
                                return r * scale[i] + offset[i];
                            };

                            //sin関数を使ってオシロスコープ感を出す
                            // (0.01 + i * 0.2) -> 変化を目立たせるため。
                            this.PlotModel.Series[i] = new FunctionSeries(convert, x, x + 4, 0.01 + i * 0.2);
                            this.PlotModel.Series[i].IsVisible = is_visible[i];
                        }
                    }
                    x += 0.1;
                    PlotModel.InvalidatePlot(true);
                    //100ミリ秒休止
                    Thread.Sleep(100);
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

            //表示したい折れ線グラフ数分、領域を確保する（今回は9）
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

        private void InitializeOffset()
        {
            for (int i = 0; i < MAX_DATA_COUNT; ++i){
                offset[i] = 0.0;
                scale[i] = 1.1;
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
    }
}