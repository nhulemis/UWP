#pragma once

using namespace Windows::UI::Notifications;
using namespace Microsoft::Toolkit::Uwp::Notifications;

class AdapterLiveTitle
{
public:
	AdapterLiveTitle();
	void SetDataInfomation();
private:
	TileUpdater^ m_tileUpdate;

	TileBinding^ m_titleWide;
	TileBinding^ m_titleMedium;
	TileBinding^ m_titleLarge;
	TileBinding^ m_titleSmall;

	TileBindingContentAdaptive^ m_contentWide;
	TileBindingContentAdaptive^ m_contentMedium;
	TileBindingContentAdaptive^ m_contentLarge;
	TileBindingContentAdaptive^ m_contentSmall;
};
