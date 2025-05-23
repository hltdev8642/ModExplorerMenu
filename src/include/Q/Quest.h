#pragma once

#include "include/B/Blacklist.h"
#include "include/D/Data.h"
#include "include/D/DataTypes.h"
#include "include/T/Table.h"
#include <PCH.h>
#include <optional>

namespace Modex
{
	class QuestWindow
	{
	public:
		static QuestWindow* GetSingleton()
		{
			static QuestWindow singleton;
			return std::addressof(singleton);
		}

		enum Viewport
		{
			TableView,
			ActiveQuestsView,
			BlacklistView
		};

		void Draw(float a_offset = 0.0f);
		void Init(bool is_default = false);
		void Load();
		void Unload();
		void Refresh();
		void BuildPluginList();

		std::vector<QuestData>& GetQuestList() { return activeQuestList; }
		::Modex::TableView<QuestData>& GetTableView() { return questTableView; }

	private:
		Viewport activeViewport = Viewport::TableView;
		std::vector<QuestData> activeQuestList;
		std::optional<QuestData> activeQuestSelection;
		bool b_ClickToAdd = true;
		int clickToAddCount = 1;

		::Modex::TableView<QuestData> questTableView;
		void ShowActions();
		void ShowQuestContextMenu(const QuestData& quest);
		void UpdateActiveQuestList();
	};
}
