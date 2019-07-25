//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"

using namespace UWPDemo;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Windows::UI::Popups;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;

/// push notification
using namespace Microsoft::Toolkit::Uwp::Notifications;
using namespace Windows::UI::Notifications;
using namespace Windows::Data::Xml::Dom;
using namespace Windows::Storage;
using namespace Concurrency;


DirectXPage::DirectXPage():
	m_windowVisible(true),
	m_coreInput(nullptr)
{
	InitializeComponent();

	// Register event handlers for page lifecycle.
	CoreWindow^ window = Window::Current->CoreWindow;

	//m_coreCursor = ref new CoreCursor(CoreCursorType::Wait, 0);

	m_result = 0;

	Platform::Collections::Vector<ItemSource::Calculation^>^ items = ref new Platform::Collections::Vector<ItemSource::Calculation^>();
	items->Append(ref new ItemSource::Calculation("Addition","+"));
	items->Append(ref new ItemSource::Calculation("Subtraction","-"));
	items->Append(ref new ItemSource::Calculation("Multiplication","*"));
	items->Append(ref new ItemSource::Calculation("Division","/"));

	cboCalculation->ItemsSource = items;

	txtPathInstalled->Text = "path installed: " + GetLocalPathInstalled();

//	cboCalculation->ItemsSource = cbo;

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXPage::OnVisibilityChanged);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDisplayContentsInvalidated);

	swapChainPanel->CompositionScaleChanged += 
		ref new TypedEventHandler<SwapChainPanel^, Object^>(this, &DirectXPage::OnCompositionScaleChanged);

	swapChainPanel->SizeChanged +=
		ref new SizeChangedEventHandler(this, &DirectXPage::OnSwapChainPanelSizeChanged);

	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	m_deviceResources = std::make_shared<DX::DeviceResources>();
	m_deviceResources->SetSwapChainPanel(swapChainPanel);

	// Register our SwapChainPanel to get independent input pointer events
	auto workItemHandler = ref new WorkItemHandler([this] (IAsyncAction ^)
	{
		// The CoreIndependentInputSource will raise pointer events for the specified device types on whichever thread it's created on.
		m_coreInput = swapChainPanel->CreateCoreIndependentInputSource(
			Windows::UI::Core::CoreInputDeviceTypes::Mouse |
			Windows::UI::Core::CoreInputDeviceTypes::Touch |
			Windows::UI::Core::CoreInputDeviceTypes::Pen
			);

		// Register for pointer events, which will be raised on the background thread.
		m_coreInput->PointerPressed += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerPressed);
		m_coreInput->PointerMoved += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerMoved);
		m_coreInput->PointerReleased += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerReleased);

		// Begin processing input messages as they're delivered.
		m_coreInput->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
	});

	// Run task on a dedicated high priority background thread.
	m_inputLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

	m_main = std::unique_ptr<UWPDemoMain>(new UWPDemoMain(m_deviceResources));
	m_main->StartRenderLoop();
}

DirectXPage::~DirectXPage()
{
	// Stop rendering and processing events on destruction.
	m_main->StopRenderLoop();
	m_coreInput->Dispatcher->StopProcessEvents();
}

// Saves the current state of the app for suspend and terminate events.
void DirectXPage::SaveInternalState(IPropertySet^ state)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->Trim();

	// Stop rendering when the app is suspended.
	m_main->StopRenderLoop();

	// Put code to save app state here.
}

// Loads the current state of the app for resume events.
void DirectXPage::LoadInternalState(IPropertySet^ state)
{
	// Put code to load app state here.

	// Start rendering when the app is resumed.
	m_main->StartRenderLoop();
}

// Window event handlers.

void DirectXPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
	if (m_windowVisible)
	{
		m_main->StartRenderLoop();
	}
	else
	{
		m_main->StopRenderLoop();
	}
}

// DisplayInformation event handlers.

void DirectXPage::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	// Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
	// if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
	// you should always retrieve it using the GetDpi method.
	// See DeviceResources.cpp for more details.
	m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->ValidateDevice();
}

// Called when the app bar button is clicked.
void DirectXPage::AppBarButton_Click(Object^ sender, RoutedEventArgs^ e)
{
	// Use the app bar if it is appropriate for your app. Design the app bar, 
	// then fill in event handlers (like this one).
}

void DirectXPage::OnPointerPressed(Object^ sender, PointerEventArgs^ e)
{
	// When the pointer is pressed begin tracking the pointer movement.
	m_main->StartTracking();
}

void DirectXPage::OnPointerMoved(Object^ sender, PointerEventArgs^ e)
{
	// Update the pointer tracking code.
	if (m_main->IsTracking())
	{
		m_main->TrackingUpdate(e->CurrentPoint->Position.X);
	}
}

void DirectXPage::OnPointerReleased(Object^ sender, PointerEventArgs^ e)
{
	// Stop tracking pointer movement when the pointer is released.
	m_main->StopTracking();
}

void DirectXPage::OnCompositionScaleChanged(SwapChainPanel^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetCompositionScale(sender->CompositionScaleX, sender->CompositionScaleY);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnSwapChainPanelSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetLogicalSize(e->NewSize);
	m_main->CreateWindowSizeDependentResources();
}

void UWPDemo::DirectXPage::ConvertStringToInt(Platform::String^ str, int & num)
{
	if (str == "")
	{
		return;
	}
	auto tem = str->Data();
	std::wstring &&wstr(tem);
	num = std::stoi(wstr);
}

bool UWPDemo::DirectXPage::isAllowInput(Windows::System::VirtualKey key)
{
	try
	{
		auto str = key.ToString();
		std::wstring &&temp(str->End() - 1);
		std::stoi(temp);
		return false;
	}
	catch (const std::exception&)
	{
		if (key == Windows::System::VirtualKey::Back)
		{
			return false;
		}

	}
	return true;
}

void UWPDemo::DirectXPage::CommandInvokedHandler(Windows::UI::Popups::IUICommand ^ command)
{

}

void UWPDemo::DirectXPage::TxtNumberOne_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	e->Handled = isAllowInput(e->Key);
}


void UWPDemo::DirectXPage::TxtNumberTwo_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	e->Handled = isAllowInput(e->Key);
}


void UWPDemo::DirectXPage::BtnResult_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	Platform::String^ cal = cboCalculation->SelectedValue->ToString();

	Platform::String^ mess = "Result of :" + txtNumberOne->Text 
		+" "+ cal +" " + txtNumberTwo->Text + " = " + m_result;

	MessageDialog^ m_msg = ref new MessageDialog(mess);
	UICommand^ cmd = ref new UICommand("close",
		ref new UICommandInvokedHandler(this, &DirectXPage::CommandInvokedHandler));
	m_msg->Commands->Append(cmd);
	m_msg->DefaultCommandIndex = 0;
	m_msg->CancelCommandIndex = 1;
	m_msg->ShowAsync();
}


void UWPDemo::DirectXPage::CboCalculation_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	int numA = 0, numB = 0;
	ConvertStringToInt(txtNumberOne->Text, numA);
	ConvertStringToInt(txtNumberOne->Text, numB);
	auto cal = cboCalculation->SelectedIndex;
	switch (cal)
	{
	case 0:
		m_result = numA + numB;
		break;
	case 1:
		m_result = numA - numB;
		break;
	case 2:
		m_result = numA * numB;
		break;
	case 3:
		m_result = float(numA) / numB;
		break;
	default:
		break;
	}
	
}

void UWPDemo::DirectXPage::Rectangle_PointerEntered(Platform::Object^ sender,
						Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	txtResult->Text = "entered";
	//Windows::UI::Input::PointerPoint^ currentPoint = e->GetCurrentPoint(recTarget);
	//Windows::UI::Core::CoreCursor::CoreCursor
	Window::Current->CoreWindow->PointerCursor = ref new CoreCursor(CoreCursorType::Custom, 104);
}


void UWPDemo::DirectXPage::Rectangle_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	txtResult->Text = "pressed";

}


void UWPDemo::DirectXPage::Rectangle_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	txtResult->Text = "released";

}


void UWPDemo::DirectXPage::Rectangle_PointerWheelChanged(Platform::Object^ sender,
						Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	auto delta = e->GetCurrentPoint(recTarget)->Properties->MouseWheelDelta;
	txtResult->Text = "wheel changed : "+delta;
}


void UWPDemo::DirectXPage::Rectangle_PointerMoved(Platform::Object^ sender,
				Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	auto pos = e->GetCurrentPoint(recTarget);
	txtResult->Text = pos->Position.X + " , " + pos->Position.Y;
}


void UWPDemo::DirectXPage::Button_Tapped(Platform::Object^ sender,
				Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	/// here show title notification
	{
		auto tileContent = ref new TileContent();
		auto tileVisual = ref new TileVisual();
		auto tileBinding = ref new TileBinding();
		auto tileBindingContentAdaptive = ref new TileBindingContentAdaptive();

		auto adaptiveText = ref new AdaptiveText();
		adaptiveText->Text = "UWP demo";
		adaptiveText->HintStyle = AdaptiveTextStyle::Subtitle;
		tileBindingContentAdaptive->Children->Append(adaptiveText);

		adaptiveText = ref new AdaptiveText();
		adaptiveText->Text = "Universal";
		adaptiveText->HintStyle = AdaptiveTextStyle::CaptionSubtle;
		tileBindingContentAdaptive->Children->Append(adaptiveText);

		adaptiveText = ref new AdaptiveText();
		adaptiveText->Text = "here is test";
		adaptiveText->HintStyle = AdaptiveTextStyle::CaptionSubtle;
		tileBindingContentAdaptive->Children->Append(adaptiveText);

		tileBinding->Content = tileBindingContentAdaptive;

		tileVisual->TileMedium = tileBinding;

		tileBinding = ref new TileBinding();
		tileBindingContentAdaptive = ref new TileBindingContentAdaptive();

		adaptiveText = ref new AdaptiveText();
		adaptiveText->Text = "Jennifer Parker";
		adaptiveText->HintStyle = AdaptiveTextStyle::Subtitle;
		tileBindingContentAdaptive->Children->Append(adaptiveText);

		adaptiveText = ref new AdaptiveText();
		adaptiveText->Text = "Photos from our trip";
		adaptiveText->HintStyle = AdaptiveTextStyle::CaptionSubtle;
		tileBindingContentAdaptive->Children->Append(adaptiveText);

		adaptiveText = ref new AdaptiveText();
		adaptiveText->Text = "Check out these awesome photos I took while in New Zealand!";
		adaptiveText->HintStyle = AdaptiveTextStyle::CaptionSubtle;
		tileBindingContentAdaptive->Children->Append(adaptiveText);

		tileBinding->Content = tileBindingContentAdaptive;

		tileVisual->TileWide = tileBinding;

		tileBinding = ref new TileBinding();
		tileBindingContentAdaptive = ref new TileBindingContentAdaptive();
		tileBinding->Content = tileBindingContentAdaptive;

		tileVisual->TileLarge = tileBinding;

		tileContent->Visual = tileVisual;

		// Create the tile notification
		auto tileNotif = ref new TileNotification(tileContent->GetXml());

		// And send the notification to the primary tile
		TileUpdateManager::CreateTileUpdaterForApplication()->Update(tileNotif);
	}

	/// here show toast notification
	{
		auto toastContent = ref new ToastContent();
		auto toastVisual = ref new ToastVisual();
		auto toastBindingGeneric = ref new ToastBindingGeneric();
		toastVisual->BindingGeneric = toastBindingGeneric;

		toastContent->Visual = toastVisual;

		// Create the toast notification
		auto toastNotif = ref new ToastNotification(toastContent->GetXml());

		// And send the notification
		ToastNotificationManager::CreateToastNotifier()->Show(toastNotif);
	}

	/// here show badge notification
	{
		m_result++;
		XmlDocument^ badgeXml = BadgeUpdateManager::GetTemplateContent(BadgeTemplateType::BadgeGlyph);
		XmlElement^ element = safe_cast<XmlElement^>(badgeXml->SelectSingleNode("/badge"));
		element->SetAttribute("value", m_result.ToString());

		BadgeNotification^ badge = ref new BadgeNotification(badgeXml);
		auto badgeUpdate = BadgeUpdateManager::CreateBadgeUpdaterForApplication();
		badgeUpdate->Update(badge);
	}
}


void UWPDemo::DirectXPage::BtnCreateFile_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	//ApplicationData::Current->LocalFolder->CreateFileAsync("uwp_text_local_path.txt", CreationCollisionOption::ReplaceExisting);
	Platform::String^ fileName = "uwp_text_local_path.txt";
	Platform::String^ folderName = "uwp_text_folder";
	auto src = ApplicationData::Current->LocalFolder;
	auto des = ApplicationData::Current->LocalCacheFolder;
	//UWPDemo::DirectXPage::CreateFileOnLocalFolder(fileName, src);
	//UWPDemo::DirectXPage::CopyFileOnLocalFolder(fileName, src, des);
	UWPDemo::DirectXPage::deleteFileOnLocalFolder(folderName, src);
}

Platform::String ^ UWPDemo::DirectXPage::GetLocalPathInstalled()
{
	return Windows::ApplicationModel::Package::Current->InstalledLocation->Path;
}

void UWPDemo::DirectXPage::CreateFileOnLocalFolder(Platform::String ^ fileName
	, Windows::Storage::StorageFolder^ src)
{
	src->CreateFileAsync(
		fileName, CreationCollisionOption::ReplaceExisting
	);
}

void UWPDemo::DirectXPage::CopyFileOnLocalFolder(Platform::String ^ fileName,
		Windows::Storage::StorageFolder^ src, Windows::Storage::StorageFolder^ des)
{
	//Create a sample file in the temporary folder
	auto copyFileTask = create_task(src->CreateFileAsync(fileName, Windows::Storage::CreationCollisionOption::OpenIfExists)).then
	([des, fileName](StorageFile^ sourceFile) -> task<StorageFile^>
	{
		//Overwrite any existing file with the same name 
		auto copyFileTask = sourceFile->CopyAsync(
			des,
			fileName,
			Windows::Storage::NameCollisionOption::ReplaceExisting
		);
		return create_task(copyFileTask);
	}).then([](StorageFile^ copiedFile) {
		//do something with copied file
	});
}

void UWPDemo::DirectXPage::deleteFileOnLocalFolder(Platform::String ^ fileName, Windows::Storage::StorageFolder^ src)
{
	create_task(src->CreateFolderAsync(
		fileName,
		Windows::Storage::CreationCollisionOption::OpenIfExists
	)).then([=](StorageFolder^ newFolder) -> task<IStorageItem^> {
		//Check the folder exists
		return create_task(src->TryGetItemAsync(fileName));
	}).then([=](IStorageItem^ newFolder) -> task<void> {

		return create_task(newFolder->DeleteAsync(StorageDeleteOption::PermanentDelete));
	}).then([=]() -> task<IStorageItem^> {

		return create_task(src->TryGetItemAsync(fileName));
	}).then([=](IStorageItem^ newFolder) {
		
	});
}
