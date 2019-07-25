#include "AdapterLiveTitle.h"
#include "pch.h"


AdapterLiveTitle::AdapterLiveTitle()
{
	m_tileUpdate = TileUpdateManager::CreateTileUpdaterForApplication();
	m_tileUpdate->EnableNotificationQueue(true);
}

void AdapterLiveTitle::SetDataInfomation()
{
	TileContent^ content = ref new TileContent();
	TileVisual^ visual = ref new TileVisual();
	content->Visual = visual;

	visual->TileWide = m_titleWide;
	visual->TileLarge = m_titleLarge;
	visual->TileSmall = m_titleSmall;
	visual->TileMedium = m_titleMedium;

	TileNotification^ titleNotif = ref new TileNotification(content->GetXml());
	titleNotif->Tag = "IBTiles";
	m_tileUpdate->Update(titleNotif);
}
