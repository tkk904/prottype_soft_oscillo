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

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //WPFアプリはAnyCPUでビルドすると実行ファイル内に32bitと64bit共存するようになります。
        //DLLにはそのような機能はありませんので
        //32bitまたは64bitかを整理してDllを動作環境下に配置しないと動作しません。
        [DllImport("math_funcs.dll")]
        private extern static double Add(double a, double b);

        [DllImport("math_funcs.dll", CallingConvention = CallingConvention.Cdecl)]
        private extern static double Subtract(double a, double b);

        [DllImport("math_funcs.dll", CallingConvention = CallingConvention.Cdecl)]
        private extern static double Multiply(double a, double b);

        public PlotModel PlotModel { get; set; }
        const int MAX_DATA_COUNT = 9;
        private bool[] is_visible = Enumerable.Repeat<bool>(true, MAX_DATA_COUNT).ToArray();

        public MainWindow()
        {
            InitializeComponent();
            
            //Demo (sin: -1.0 -> 1.0)
            this.PlotModel = CreatePlotModel(-1, 1);

            DataContext = this;
            var worker = new BackgroundWorker { WorkerSupportsCancellation = true };
            double x = 0;
            worker.DoWork += (s, e) =>
            {
                //バックグラウンドワーカーを作成する（作業スレッドを作成すると同じ）
                //ここでセンサーから値を取得する？
                while (!worker.CancellationPending)
                {
                    lock (this.PlotModel.SyncRoot)
                    {
                        this.PlotModel.Title = "Plot updated: " + DateTime.Now;
                        for (int i = 0; i < MAX_DATA_COUNT; ++i) {

                            //sin関数を使ってオシロスコープ感を出す
                            // (0.01 + i * 0.2) -> 変化を目立たせるため。
                            this.PlotModel.Series[i] = new FunctionSeries(Math.Sin, x, x + 4, 0.01 + Multiply((double) i , 0.2));
                            this.PlotModel.Series[i].IsVisible = is_visible[i];
                        }
                    }
                    x = Add(x,0.1);
                    PlotModel.InvalidatePlot(true);
                    //100ミリ秒休止
                    Thread.Sleep(100);
                }
            };
            worker.RunWorkerAsync();
            this.Closed += (s, e) => worker.CancelAsync();
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
            var index = get_checkbox_index(sender);
            //チェックボックスで表示可否を操作する
            is_visible[index] = (sender as CheckBox)?.IsChecked ?? false;
        }

        private int get_checkbox_index(object sender)
        {
            //チェックボックスの名前からインデックスを取得する
            var ck = (sender as CheckBox);
            String number = ck.Name;
            return int.Parse(number.Substring(number.Length - 1, 1));
        }
    }
}