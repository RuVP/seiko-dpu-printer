using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration;
using System.Drawing;
using System.Drawing.Printing;
using System.IO;
using System.Linq;
using System.Windows;
using Microsoft.Win32;
using Prism.Commands;
using SeikoPrinter;

namespace PrinterTester.ViewModel
{
    internal class MainWindowViewModel : INotifyPropertyChanged
    {
        private static readonly PrinterHelperWrapper printerHelper = new PrinterHelperWrapper();
        private static readonly SlipReceipt seikoPrinterReceipt = new SlipReceipt();

        private const string NotApplicable = "N/A";

        #region Properties

        private string _printContent = ConfigurationManager.AppSettings["PrintContent"];

        public string PrintContent
        {
            get { return _printContent; }
            set
            {
                _printContent = value;
                OnPropertyChanged("PrintContent");
            }
        }

        private string _logoPath = ConfigurationManager.AppSettings["LogoPath"];

        public string LogoPath
        {
            get { return _logoPath; }
            set
            {
                _logoPath = value;
                OnPropertyChanged("LogoPath");
            }
        }

        private string _printerStatus = NotApplicable;

        public string PrinterStatus
        {
            get { return _printerStatus; }
            set
            {
                _printerStatus = value;
                OnPropertyChanged("PrinterStatus");
            }
        }

        private List<string> _printerList = new List<string>();

        public List<string> PrinterList
        {
            get { return _printerList; }
            set
            {
                _printerList = value;
                OnPropertyChanged("PrinterList");
            }
        }

        private string _selectedPrinterName = ConfigurationManager.AppSettings["SelectedPrinterName"];

        public string SelectedPrinterName
        {
            get { return _selectedPrinterName; }
            set
            {
                _selectedPrinterName = value;
                OnPropertyChanged("SelectedPrinterName");
            }
        }

        private bool _isPrinterConnected = false;

        public bool IsPrinterConnected
        {
            get { return _isPrinterConnected; }
            set
            {
                _isPrinterConnected = value;
                OnPropertyChanged("IsPrinterConnected");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged([System.Runtime.CompilerServices.CallerMemberName] string propertyName = "unknown")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        #endregion Properties

        #region Commands

        private DelegateCommand _refreshPrinterListCommand;

        public DelegateCommand RefreshPrinterListCommand
        {
            get
            {
                return _refreshPrinterListCommand
                          ?? (_refreshPrinterListCommand = new DelegateCommand(ExecuteRefreshPrinterList));
            }
        }

        private DelegateCommand _connectPrinterCommand;

        public DelegateCommand ConnectPrinterCommand
        {
            get
            {
                return _connectPrinterCommand
                          ?? (_connectPrinterCommand = new DelegateCommand(ExecuteConnectPrinter));
            }
        }

        private DelegateCommand _printCommand;

        public DelegateCommand PrintCommand
        {
            get
            {
                return _printCommand
                          ?? (_printCommand = new DelegateCommand(ExecutePrint));
            }
        }

        private DelegateCommand _printerResetCommand;

        public DelegateCommand PrinterResetCommand
        {
            get
            {
                return _printerResetCommand
                          ?? (_printerResetCommand = new DelegateCommand(ExecutePrinterReset));
            }
        }

        private DelegateCommand _selectLogoCommand;

        public DelegateCommand SelectLogoCommand
        {
            get
            {
                return _selectLogoCommand
                          ?? (_selectLogoCommand = new DelegateCommand(SelectLogo));
            }
        }

        private DelegateCommand _getPrintreStatusCommand;

        public DelegateCommand GetPrintreStatusCommand
        {
            get
            {
                return _getPrintreStatusCommand
                          ?? (_getPrintreStatusCommand = new DelegateCommand(ExecuteGetPrintreStatusCommand));
            }
        }

        #endregion Commands

        public MainWindowViewModel()
        {
            try
            {
                foreach (string printerName in PrinterSettings.InstalledPrinters)
                {
                    PrinterList.Add(printerName);
                }
                PrinterList = PrinterList.OrderBy(x => x).ToList();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error");
            }
        }

        private void ExecuteRefreshPrinterList()
        {
            try
            {
                PrinterList.Clear();
                foreach (string printerName in PrinterSettings.InstalledPrinters)
                {
                    PrinterList.Add(printerName);
                }
                PrinterList = PrinterList.OrderBy(x => x).ToList();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error");
            }
        }

        private void ExecuteGetPrintreStatusCommand()
        {
            try
            {
                PrinterStatus = printerHelper.GetPrinterStatusCode().ToString();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error");
            }
        }
        

        private void ExecuteConnectPrinter()
        {
            try
            {
                IsPrinterConnected = printerHelper.ConnectPrinterDevice(SelectedPrinterName);

                if (!IsPrinterConnected)
                {
                    MessageBox.Show("Printer not available.", "Error");
                }
            }
            catch (Win32Exception wex)
            {
                MessageBox.Show(wex.Message, "Win32 Error");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error");
            }
        }

        private void ExecutePrint()
        {
            try
            {
                // If you want to mandate a logo print, uncomment these lines.
                //if (!File.Exists(LogoPath))
                //{
                //    MessageBox.Show("File mentioned in Logo does not exist.", "Error");
                //    return;
                //}

                // Below code is not tested, since I did not have the hardware.
                if (File.Exists(LogoPath))
                {
                    using (Image image = Image.FromFile(LogoPath))
                    {
                        using (MemoryStream m = new MemoryStream())
                        {
                            image.Save(m, image.RawFormat);
                            byte[] imageBytes = m.ToArray();

                            // Convert byte[] to Base64 String
                            string base64String = Convert.ToBase64String(imageBytes);
                            seikoPrinterReceipt.logo = base64String;
                        }
                    }
                }

                seikoPrinterReceipt.content = PrintContent;
                seikoPrinterReceipt.printDateTime = DateTime.Now.ToString("G");

                printerHelper.PrintSlipReceipt(seikoPrinterReceipt);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error");
            }
        }

        private void ExecutePrinterReset()
        {
            try
            {
                printerHelper.ResetPrinterDevice();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error");
            }
        }

        private void SelectLogo()
        {
            try
            {
                var openFileDialog = new OpenFileDialog();
                if (openFileDialog.ShowDialog() == true)
                {
                    LogoPath = openFileDialog.FileName;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error");
            }
        }
    }
}
