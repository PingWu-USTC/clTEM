﻿using System;
using System.IO;
using System.ComponentModel;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.Threading;
using Microsoft.Win32;
using ManagedOpenCLWrapper;
using BitMiracle.LibTiff.Classic;
using PanAndZoom;
using ColourGenerator;

namespace GPUTEMSTEMSimulation
{
    public partial class MainWindow : Elysium.Controls.Window
    {

        private void ComboBoxSelectionChanged1(object sender, SelectionChangedEventArgs e)
        {
            Resolution = Convert.ToInt32(ResolutionCombo.SelectedValue.ToString());

            IsResolutionSet = true;

            if (!userSTEMarea)
            {
                STEMRegion.xPixels = Resolution;
                STEMRegion.yPixels = Resolution;
            }

            UpdatePx();
        }

        private void UpdatePx()
        {
            if (HaveStructure && IsResolutionSet)
            {
                var BiggestSize = Math.Max(SimRegion.xFinish - SimRegion.xStart, SimRegion.yFinish - SimRegion.yStart);
                pixelScale = BiggestSize / Resolution;
                PixelScaleLabel.Content = pixelScale.ToString("f2") + " Å";

                UpdateMaxMrad();
            }
        }

        private void UpdateMaxMrad()
        {

            if (!HaveStructure)
                return;

            var MinX = SimRegion.xStart;
            var MinY = SimRegion.yStart;

            var MaxX = SimRegion.xFinish;
            var MaxY = SimRegion.yFinish;

            var BiggestSize = Math.Max(MaxX - MinX, MaxY - MinY);
            // Determine max mrads for reciprocal space, (need wavelength)...
            var MaxFreq = 1 / (2 * BiggestSize / Resolution);

            if (ImagingParameters.kilovoltage != 0 && IsResolutionSet)
            {
                const float echarge = 1.6e-19f;
                wavelength = Convert.ToSingle(6.63e-034 * 3e+008 / Math.Sqrt((echarge * ImagingParameters.kilovoltage * 1000 * 
                    (2 * 9.11e-031 * 9e+016 + echarge * ImagingParameters.kilovoltage * 1000))) * 1e+010);

                var mrads = (1000 * MaxFreq * wavelength) / 2; //divide by two to get mask limits

                MaxMradsLabel.Content = mrads.ToString("f2")+" mrad";

                HaveMaxMrad = true;
            }
        }

        private void ImagingDf_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingDf.Text;
            float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.df);
            float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.df);
        }

        private void ImagingCs_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingCs.Text;
            float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.spherical);
            float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.spherical);
        }

        private void ImagingA1_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingA1.Text;
            float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.astigmag);
            float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.astigmag);
        }

        private void ImagingA1theta_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingA1theta.Text;
            var ok = false;

            ok = float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.astigang);
            ok |= float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.astigang);

            if (ok)
            {
                ImagingParameters.astigang /= Convert.ToSingle((180 / Math.PI));
                ProbeParameters.astigang /= Convert.ToSingle((180 / Math.PI));
            }


        }

        private void ImagingkV_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingkV.Text;

            if (temporarytext.Length == 0)
                return;

            float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.kilovoltage);
            float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.kilovoltage);

            UpdateMaxMrad();
        }

        private void Imagingbeta_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = Imagingbeta.Text;
            var ok = false;

            ok = float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.beta);
            ok |= float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.beta);

            if (ok)
            {
                ImagingParameters.beta /= 1000;
                ProbeParameters.beta /= 1000;
            }
        }

        private void Imagingdelta_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = Imagingdelta.Text;
            var ok = false;

            ok = float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.delta);
            ok |= float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.delta);

            if (ok)
            {
                ImagingParameters.delta *= 10;
                ProbeParameters.delta *= 10;
            }
        }

        private void ImagingAperture_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingAperture.Text;

            if (temporarytext.Length == 0)
                return;

            float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.aperturemrad);
            float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.aperturemrad);
        }

        private void SimTypeRadio_Checked(object sender, RoutedEventArgs e)
        {
            if (TEMRadioButton.IsChecked == true)
            {
                ImagingA2.IsEnabled = true;
                ImagingA2Phi.IsEnabled = true;
                ImagingB2.IsEnabled = true;
                ImagingB2Phi.IsEnabled = true;
                Imagingdelta.IsEnabled = true;
                Imagingbeta.IsEnabled = true;
                TEMbox.Visibility = System.Windows.Visibility.Visible;
                STEMbox.Visibility = System.Windows.Visibility.Hidden;
                CBEDbox.Visibility = System.Windows.Visibility.Hidden;

            }
            else if (STEMRadioButton.IsChecked == true)
            {
                ImagingA2.IsEnabled = false;
                ImagingA2Phi.IsEnabled = false;
                ImagingB2.IsEnabled = false;
                ImagingB2Phi.IsEnabled = false;
                Imagingdelta.IsEnabled = false;
                Imagingbeta.IsEnabled = false;

                STEMbox.Visibility = System.Windows.Visibility.Visible;
                TEMbox.Visibility = System.Windows.Visibility.Hidden;
                CBEDbox.Visibility = System.Windows.Visibility.Hidden;
            }
            else if (CBEDRadioButton.IsChecked == true)
            {
                ImagingA2.IsEnabled = false;
                ImagingA2Phi.IsEnabled = false;
                ImagingB2.IsEnabled = false;
                ImagingB2Phi.IsEnabled = false;
                Imagingdelta.IsEnabled = false;
                Imagingbeta.IsEnabled = false;

                STEMbox.Visibility = System.Windows.Visibility.Hidden;
                TEMbox.Visibility = System.Windows.Visibility.Hidden;
                CBEDbox.Visibility = System.Windows.Visibility.Visible;
            }
        }

        private void ImagingB2_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingAperture.Text;
            float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.b2mag);
        }

        private void ImagingB2Phi_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingAperture.Text;
            var ok = false;

            ok = float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.b2ang);
            ok |= float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.b2ang);

            if (ok)
            {
                ImagingParameters.b2ang /= Convert.ToSingle((180 / Math.PI));
                ProbeParameters.b2ang /= Convert.ToSingle((180 / Math.PI));
            }
        }

        private void ImagingA2Phi_TextChanged(object sender, TextChangedEventArgs e)
        {

            var temporarytext = ImagingAperture.Text;
            var ok = false;

            ok = float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.astig2ang);
            ok |= float.TryParse(temporarytext, NumberStyles.Float, null, out ProbeParameters.astig2ang);

            if (ok)
            {
                ImagingParameters.astig2ang /= Convert.ToSingle((180 / Math.PI));
                ProbeParameters.astig2ang /= Convert.ToSingle((180 / Math.PI));
            }
        }

        private void ImagingA2_TextChanged(object sender, TextChangedEventArgs e)
        {
            var temporarytext = ImagingAperture.Text;
            float.TryParse(temporarytext, NumberStyles.Float, null, out ImagingParameters.astig2mag);
        }

        private void STEM_TDSchecked(object sender, RoutedEventArgs e)
        {
            doTDS_STEM = true;
        }

        private void STEM_TDSunchecked(object sender, RoutedEventArgs e)
        {
            doTDS_STEM = false;
        }

        private void CBED_TDSchecked(object sender, RoutedEventArgs e)
        {
            doTDS_CBED = true;
        }

        private void CBED_TDSunchecked(object sender, RoutedEventArgs e)
        {
            doTDS_CBED = false;
        }

        private void Full3D_Checked(object sender, RoutedEventArgs e)
        {
            isFull3D = true;
            if(ToggleFD != null)
            { ToggleFD.IsChecked = false; }   
            isFD = false;
        }

        private void Full3D_Unchecked(object sender, RoutedEventArgs e)
        {
            isFull3D = false;
        }

        private void FD_Checked(object sender, RoutedEventArgs e)
        {
            // Cant use full3d at same time
            isFull3D = false;
            if(ToggleFull3D!=null)
                ToggleFull3D.IsChecked = false;
            isFD = true;
        }

        private void FD_Unchecked(object sender, RoutedEventArgs e)
        {
            isFD = false;
        }

        private void Show_detectors(object sender, RoutedEventArgs e)
        {
            foreach (var i in Detectors)
            {
                i.SetVisibility(true);
            }
            DetectorVis = true;
        }

        private void Hide_Detectors(object sender, RoutedEventArgs e)
        {
            foreach (var i in Detectors)
            {
                i.SetVisibility(false);
            }
            DetectorVis = false;
        }

        private void STEMDet_Click(object sender, RoutedEventArgs e)
        {
            // open the window here
            var window = new STEMDetectorDialog(Detectors) {Owner = this};
            window.AddDetectorEvent += new EventHandler<DetectorArgs>(STEM_AddDetector);
            window.RemDetectorEvent += new EventHandler<DetectorArgs>(STEM_RemoveDetector);
            window.ShowDialog();
        }

        private void STEMArea_Click(object sender, RoutedEventArgs e)
        {
            var window = new STEMAreaDialog(STEMRegion, SimRegion) {Owner = this};
            window.AddSTEMAreaEvent += new EventHandler<StemAreaArgs>(STEM_AddArea);
            window.ShowDialog();
        }

        void STEM_AddDetector(object sender, DetectorArgs evargs)
        {
            var added = evargs.Detector as DetectorItem;
            LeftTab.Items.Add(added.Tab);
            added.AddToCanvas(DiffDisplay.tCanvas);
            if(HaveMaxMrad)
                added.SetEllipse(CurrentResolution, CurrentPixelScale, CurrentWavelength, DetectorVis);
        }

        void STEM_RemoveDetector(object sender, DetectorArgs evargs)
        {
            foreach (var i in evargs.DetectorList)
            {
                i.RemoveFromCanvas(DiffDisplay.tCanvas);
                LeftTab.Items.Remove(i.Tab);
            }

            foreach (var i in Detectors)
            {
                i.SetColour();//Ellipse(CurrentResolution, CurrentPixelScale, CurrentWavelength, DetectorVis);
            }
        }

        void STEM_AddArea(object sender, StemAreaArgs evargs)
        {
            userSTEMarea = true;
            STEMRegion = evargs.AreaParams;
        }

        private void DeviceSelector_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            //var CB = sender as ComboBox;
            //mCL.SetDevice(CB.SelectedIndex);
        }

        private void DeviceSelector_DropDownOpened(object sender, EventArgs e)
        {
            DeviceSelector.ItemsSource = devicesLong;
        }

        private void DeviceSelector_DropDownClosed(object sender, EventArgs e)
        {
            var CB = sender as ComboBox;

            var index = CB.SelectedIndex;
            CB.ItemsSource = devicesShort;
            CB.SelectedIndex = index;
            if (index != -1) // Later, might want to check for index the same as before
            {
                mCL.setCLdev(CB.SelectedIndex);
                //CB.IsEnabled = false;
				SimulateImageButton.IsEnabled = false;
            }
        }

        private void SetAreaButton_Click(object sender, RoutedEventArgs e)
        {
            var window = new AreaDialog(SimRegion) {Owner = this};
            window.SetAreaEvent += new EventHandler<AreaArgs>(SetArea);
            window.ShowDialog();
        }

        void SetArea(object sender, AreaArgs evargs)
        {
            var changedx = false;
            var changedy = false;
            userSIMarea = true;
            SimRegion = evargs.AreaParams;

            var xscale = (STEMRegion.xStart - STEMRegion.xFinish) / STEMRegion.xPixels;
            var yscale = (STEMRegion.yStart - STEMRegion.yFinish) / STEMRegion.yPixels;

            if (STEMRegion.xStart < SimRegion.xStart || STEMRegion.xStart > SimRegion.xFinish)
            {
                STEMRegion.xStart = SimRegion.xStart;
                changedx = true;
            }

            if (STEMRegion.xFinish > SimRegion.xFinish || STEMRegion.xFinish < SimRegion.xStart)
            {
                STEMRegion.xFinish = SimRegion.xFinish;
                changedx = true;
            }

            if (STEMRegion.yStart < SimRegion.yStart || STEMRegion.yStart > SimRegion.yFinish)
            {
                STEMRegion.yStart = SimRegion.yStart;
                changedy = true;
            }

            if (STEMRegion.yFinish > SimRegion.yFinish || STEMRegion.yFinish < SimRegion.yStart)
            {
                STEMRegion.yFinish = SimRegion.yFinish;
                changedy = true;
            }

            if (changedx)
                STEMRegion.xPixels = (int)Math.Ceiling((STEMRegion.xStart - STEMRegion.xFinish) / xscale);

            if (changedy)
                STEMRegion.yPixels = (int)Math.Ceiling((STEMRegion.yStart - STEMRegion.yFinish) / yscale);

            //var result = MessageBox.Show("STEM limits now out of bounds and have been rescaled", "", MessageBoxButton.OK, MessageBoxImage.Error);
			UpdatePx();
            //UpdateMaxMrad();
        }

        private void GridZoomReset(object sender, MouseButtonEventArgs e)
        {
            var tempGrid = sender as Grid;

            var child = VisualTreeHelper.GetChild(tempGrid, 0) as ZoomBorder;
            if (child != null)
                child.Reset();
        }

		// Test if conditions necessary to perform simulation have been met.s
		private bool TestSimulationPrerequisites()
		{
			// Check We Have Structure
			if (HaveStructure == false)
			{
				var result = MessageBox.Show("No Structure Loaded", "", MessageBoxButton.OK, MessageBoxImage.Error);
				return false;
			}
			// Check parameters are set
			if (IsResolutionSet == false)
			{
				var result = MessageBox.Show("Resolution Not Set", "", MessageBoxButton.OK, MessageBoxImage.Error);
				return false;
			}
			// Check for OpenCL device.
			if (DeviceSelector.SelectedIndex == -1)
			{
				var result = MessageBox.Show("OpenCL Device Not Set", "", MessageBoxButton.OK, MessageBoxImage.Error);
				return false;
			}
			// Check we have sensible parameters.
			if (ImagingParameters.kilovoltage == 0)
			{
				var result = MessageBox.Show("Voltage cannot be zero", "", MessageBoxButton.OK, MessageBoxImage.Error);
				return false;
			}
			if (ImagingParameters.aperturemrad == 0)
			{
				var result = MessageBox.Show("Aperture should not be zero, do you want to continue?", "Continue?", MessageBoxButton.YesNoCancel, MessageBoxImage.Error);
				return result.Equals(MessageBoxResult.Yes);
			}
            if (!CBED_posValid && select_CBED)
            {
                var result = MessageBox.Show("CBED probe position outside simulated region", "", MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }
            if (ToggleFD.IsChecked == true && !goodfinite)
            {
                var result = MessageBox.Show("Incorrect finite difference settings", "", MessageBoxButton.OK, MessageBoxImage.Error);
                return false;
            }
			return true;
		}

		private bool TestImagePrerequisites()
		{
			if (ImagingParameters.aperturemrad == 0)
			{
				var result = MessageBox.Show("Aperture should not be zero, do you want to continue?", "Continue?", MessageBoxButton.YesNoCancel, MessageBoxImage.Error);
				return result.Equals(MessageBoxResult.Yes);
			}
			else return true;
		}

        private void Cancel_Click(object sender, RoutedEventArgs e)
        {
            cancellationTokenSource.Cancel();
        }

        private void FiniteValidCheck(object sender, TextChangedEventArgs e)
        {
            var tbox = sender as TextBox;
            var text = tbox.Text;

            if (text.Length < 1 || text == ".")
            {
                tbox.Background = (SolidColorBrush)Application.Current.Resources["ErrorCol"];
                goodfinite = false;
            }
            else
            {
                tbox.Background = (SolidColorBrush)Application.Current.Resources["TextBoxBackground"];
                goodfinite = goodfinite && true;
            }
        }

        //private void CBEDValidCheck(object sender, TextCompositionEventArgs e)
        private void CBEDValidCheck(object sender, TextChangedEventArgs e)
        {
            var tbox = sender as TextBox;

            float lower;
            float upper;
            var isx = false;

            if (tbox == CBEDxpos)
            {
                lower = SimRegion.xStart;
                upper = SimRegion.xFinish;
                isx = true;
            }
            else if (tbox == CBEDypos)
            {
                lower = SimRegion.yStart;
                upper = SimRegion.yFinish;
            }
            else
                return;

            var text = tbox.Text;

            if (text.Length < 1 || text == ".")
                CBED_posValid = false;
            else
                CBED_posValid = true;

            if (CBED_posValid)
            {
                var pos = Convert.ToSingle(text);

                if (isx)
                    CBED_xpos = pos;
                else
                    CBED_xpos = pos;

                CBED_posValid = pos >= lower && pos <= upper;
            }

            if (!CBED_posValid)
                tbox.Background = (SolidColorBrush)Application.Current.Resources["ErrorCol"];
            else
                tbox.Background = (SolidColorBrush)Application.Current.Resources["TextBoxBackground"];
        }
    }
}
