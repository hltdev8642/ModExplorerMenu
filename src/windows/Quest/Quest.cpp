#include "include/Q/Quest.h"
#include "include/B/Blacklist.h"
#include "include/C/Console.h"
#include "include/D/Data.h"
#include "include/M/Menu.h"
#include "include/S/Settings.h"
#include "include/U/Util.h"

namespace Modex
{
	void QuestWindow::Draw(float a_offset)
	{
		const float MIN_SEARCH_HEIGHT = 175.0f;
		const float MIN_SEARCH_WIDTH = 200.0f;
		const float MAX_SEARCH_HEIGHT = ImGui::GetContentRegionAvail().y * 0.75f;
		const float MAX_SEARCH_WIDTH = ImGui::GetContentRegionAvail().x * 0.85f;

		const ImGuiChildFlags flags = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;
		float search_height = ImGui::GetStateStorage()->GetFloat(ImGui::GetID("Quest::SearchHeight"), MIN_SEARCH_HEIGHT);
		float search_width = ImGui::GetStateStorage()->GetFloat(ImGui::GetID("Quest::SearchWidth"), MAX_SEARCH_WIDTH);
		float window_padding = ImGui::GetStyle().WindowPadding.y;
		const float button_width = ImGui::GetContentRegionMax().x / 3.0f;
		const float button_height = ImGui::GetFontSize() * 1.5f;
		const float tab_bar_height = button_height + (window_padding * 2);

		ImGui::SetCursorPosY(window_padding);
		ImGui::SetCursorPosX(window_padding + a_offset);
		ImVec2 backup_pos = ImGui::GetCursorPos();

		// Tab Button Area
		if (ImGui::BeginChild("##Quest::Tabs", ImVec2(0.0f, button_height), 0, ImGuiWindowFlags_NoFocusOnAppearing)) {
			ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));

			if (ImGui::Selectable("Table View", activeViewport == Viewport::TableView, 0, ImVec2(button_width, 0.0f))) {
				activeViewport = Viewport::TableView;
				if (this->questTableView.GetTableList().empty()) {
					this->questTableView.Refresh();
				}
				this->questTableView.BuildPluginList();
			}

			ImGui::SameLine();

			if (ImGui::Selectable("Active Quests", activeViewport == Viewport::ActiveQuestsView, 0, ImVec2(button_width, 0.0f))) {
				activeViewport = Viewport::ActiveQuestsView;
				this->UpdateActiveQuestList();
			}

			ImGui::SameLine();

			if (ImGui::Selectable("Blacklist", activeViewport == Viewport::BlacklistView, 0, ImVec2(button_width, 0.0f))) {
				activeViewport = Viewport::BlacklistView;
				Blacklist::GetSingleton()->BuildPluginList();
			}

			ImGui::PopStyleVar();
		}
		ImGui::EndChild();

		if (activeViewport == Viewport::BlacklistView) {
			ImGui::SetCursorPos(backup_pos);
			ImGui::SetCursorPosY(tab_bar_height - window_padding);
			Blacklist::GetSingleton()->Draw(0.0f);
		} else if (activeViewport == Viewport::ActiveQuestsView) {
			// Active Quests View
			ImGui::SetCursorPos(backup_pos);
			ImGui::SetCursorPosY(tab_bar_height - window_padding);

			// Split the view into two parts: quest list and quest details/actions
			const float splitWidth = ImGui::GetContentRegionAvail().x * 0.7f;

			// Quest List Area
			if (ImGui::BeginChild("##Quest::ActiveQuestList", ImVec2(splitWidth, 0), true)) {
				if (activeQuestList.empty()) {
					UpdateActiveQuestList();
				}

				if (activeQuestList.empty()) {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No active quests found!");
				} else {
					// Create a table for active quests
					if (ImGui::BeginTable("##ActiveQuestsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)) {
						ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
						ImGui::TableSetupColumn("Stage", ImGuiTableColumnFlags_WidthFixed, 60.0f);
						ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
						ImGui::TableHeadersRow();

						for (const auto& quest : activeQuestList) {
							ImGui::TableNextRow();

							// Name column
							ImGui::TableNextColumn();
							std::string questName = quest.GetName();
							std::string questLabel = questName + "##" + quest.GetFormID();

							bool isSelected = (activeQuestSelection.has_value() &&
								activeQuestSelection->GetFormID() == quest.GetFormID());

							if (ImGui::Selectable(questLabel.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
								activeQuestSelection = quest;

								if (ImGui::IsMouseDoubleClicked(0) && b_ClickToAdd) {
									Console::ExecuteQuestCommand("getstage", quest.GetFormID());
									Console::StartProcessThread();
								}
							}

							// Status column
							ImGui::TableNextColumn();
							std::string questStatus = "Unknown";
							if (quest.IsCompleted()) {
								questStatus = "Completed";
								ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), questStatus.c_str());
							} else if (quest.IsActive()) {
								questStatus = "Active";
								ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), questStatus.c_str());
							} else if (quest.IsStarting()) {
								questStatus = "Starting";
								ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), questStatus.c_str());
							} else if (quest.IsStopping()) {
								questStatus = "Stopping";
								ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), questStatus.c_str());
							} else {
								ImGui::Text(questStatus.c_str());
							}

							// Stage column
							ImGui::TableNextColumn();
							ImGui::Text("%d", quest.GetCurrentStage());

							// Type column
							ImGui::TableNextColumn();
							ImGui::Text("%s", quest.GetQuestType().c_str());

							// Context menu for quest actions
							if (ImGui::BeginPopupContextItem()) {
								ShowQuestContextMenu(quest);
								ImGui::EndPopup();
							}
						}
						ImGui::EndTable();
					}
				}
				ImGui::EndChild();
			}

			// Quest Details and Actions Area
			ImGui::SameLine();
			if (ImGui::BeginChild("##Quest::ActiveQuestDetails", ImVec2(0, 0), true)) {
				if (activeQuestSelection.has_value()) {
					const auto& quest = activeQuestSelection.value();

					// Quest Information Section
					if (ImGui::CollapsingHeader("Quest Information", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::Text("Name: %s", quest.GetName().c_str());
						ImGui::Text("Editor ID: %s", quest.GetEditorID().c_str());
						ImGui::Text("Form ID: %s", quest.GetFormID().c_str());
						ImGui::Text("Plugin: %s", quest.GetPluginName().c_str());
						ImGui::Text("Type: %s", quest.GetQuestType().c_str());
						ImGui::Text("Current Stage: %d", quest.GetCurrentStage());

						std::string status = "Unknown";
						if (quest.IsCompleted()) {
							status = "Completed";
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: %s", status.c_str());
						} else if (quest.IsActive()) {
							status = "Active";
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: %s", status.c_str());
						} else if (quest.IsStarting()) {
							status = "Starting";
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Status: %s", status.c_str());
						} else if (quest.IsStopping()) {
							status = "Stopping";
							ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Status: %s", status.c_str());
						} else {
							ImGui::Text("Status: %s", status.c_str());
						}
					}

					// Quest Commands Section
					if (ImGui::CollapsingHeader("Quest Commands", ImGuiTreeNodeFlags_DefaultOpen)) {
						if (ImGui::Button("Get Stage", ImVec2(-1, 0))) {
							Console::ExecuteQuestCommandWithMessageBox("getstage", quest.GetFormID());
						}

						if (ImGui::Button("Show Quest Stages", ImVec2(-1, 0))) {
							Console::ExecuteQuestCommandWithMessageBox("sqs", quest.GetFormID());
						}

						if (ImGui::Button("Show Quest Variables", ImVec2(-1, 0))) {
							Console::ExecuteQuestCommandWithMessageBox("sqv", quest.GetFormID());
						}

						if (ImGui::Button("Move To Quest Target", ImVec2(-1, 0))) {
							Console::ExecuteQuestCommandWithMessageBox("movetoqt", quest.GetFormID());
							Menu::GetSingleton()->Close();
						}

						ImGui::Separator();

						if (ImGui::Button("Complete Quest", ImVec2(-1, 0))) {
							Console::ExecuteQuestCommandWithMessageBox("completequest", quest.GetFormID());
							Menu::GetSingleton()->Close();
						}

						if (ImGui::Button("Reset Quest", ImVec2(-1, 0))) {
							Console::ExecuteQuestCommandWithMessageBox("resetquest", quest.GetFormID());
							Menu::GetSingleton()->Close();
						}

						ImGui::Separator();

						static int stageValue = 10;
						ImGui::SetNextItemWidth(-1);
						ImGui::InputInt("Stage Value", &stageValue, 5);

						if (ImGui::Button("Set Stage", ImVec2(-1, 0))) {
							Console::ExecuteQuestCommandWithMessageBox("setstage", quest.GetFormID(), stageValue);
							Menu::GetSingleton()->Close();
						}
					}
				} else {
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a quest to view details and actions");
				}
				ImGui::EndChild();
			}
		} else if (activeViewport == Viewport::TableView) {
			// Search Input Area
			ImGui::SetCursorPos(backup_pos);
			ImGui::SetCursorPosY(tab_bar_height - window_padding);
			backup_pos = ImGui::GetCursorPos();
			if (ImGui::BeginChild("##Quest::SearchArea", ImVec2(search_width - a_offset, search_height), flags, ImGuiWindowFlags_NoFocusOnAppearing)) {
				this->questTableView.ShowSearch(search_height);
			}
			ImGui::EndChild();

			// Horizontal Search / Table Splitter
			float full_width = search_width - a_offset;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + a_offset);
			ImGui::SetCursorPosY(backup_pos.y + search_height);
			ImGui::DrawSplitter("##Quest::HorizontalSplitter", true, &search_height, &full_width, MIN_SEARCH_HEIGHT, MAX_SEARCH_HEIGHT);

			// Table Area
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + a_offset);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (window_padding / 2));
			if (ImGui::BeginChild("##Quest::TableArea", ImVec2(search_width - a_offset, 0), flags, ImGuiWindowFlags_NoFocusOnAppearing)) {
				this->questTableView.ShowSort();
				this->questTableView.Draw();
			}
			ImGui::EndChild();

			// Vertical Search Table / Action Splitter
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - window_padding);
			ImGui::SetCursorPosY(tab_bar_height - window_padding);
			float full_height = ImGui::GetContentRegionAvail().y;
			ImGui::DrawSplitter("##Quest::VerticalSplitter2", false, &search_width, &full_height, MIN_SEARCH_WIDTH, MAX_SEARCH_WIDTH);

			// Action Area
			ImGui::SameLine();
			ImGui::SetCursorPosY(tab_bar_height - window_padding);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - window_padding);
			if (ImGui::BeginChild("##Quest::ActionArea",
					ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), flags, ImGuiWindowFlags_NoFocusOnAppearing)) {
				this->ShowActions();
			}
			ImGui::EndChild();

			// Persist Search Area Width/Height
			ImGui::GetStateStorage()->SetFloat(ImGui::GetID("Quest::SearchWidth"), search_width);
			ImGui::GetStateStorage()->SetFloat(ImGui::GetID("Quest::SearchHeight"), search_height);
		}
	}

	void QuestWindow::ShowActions()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 8));

		ImGui::Text("Quest Actions");
		ImGui::Separator();

		const auto selection = questTableView.GetSelection();
		const bool hasSelection = !selection.empty();

		if (ImGui::CollapsingHeader("Quest Information", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (hasSelection) {
				const auto& quest = selection[0];

				ImGui::Text("Name: %s", quest.GetName().c_str());
				ImGui::Text("Editor ID: %s", quest.GetEditorID().c_str());
				ImGui::Text("Form ID: %s", quest.GetFormID().c_str());
				ImGui::Text("Type: %s", quest.GetQuestType().c_str());
				ImGui::Text("Current Stage: %d", quest.GetCurrentStage());

				std::string status = "Unknown";
				if (quest.IsCompleted()) {
					status = "Completed";
				} else if (quest.IsActive()) {
					status = "Active";
				} else if (quest.IsStarting()) {
					status = "Starting";
				} else if (quest.IsStopping()) {
					status = "Stopping";
				}
				ImGui::Text("Status: %s", status.c_str());
			} else {
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a quest to view details");
			}
		}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Quest Commands", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BeginDisabled(!hasSelection);

			if (ImGui::Button("Get Stage", ImVec2(-1, 0))) {
				if (hasSelection) {
					Console::ExecuteQuestCommandWithMessageBox("getstage", selection[0].GetFormID());
				}
			}

			if (ImGui::Button("Show Quest Stages", ImVec2(-1, 0))) {
				if (hasSelection) {
					Console::ExecuteQuestCommandWithMessageBox("sqs", selection[0].GetFormID());
				}
			}

			if (ImGui::Button("Show Quest Variables", ImVec2(-1, 0))) {
				if (hasSelection) {
					Console::ExecuteQuestCommandWithMessageBox("sqv", selection[0].GetFormID());
				}
			}

			if (ImGui::Button("Move To Quest Target", ImVec2(-1, 0))) {
				if (hasSelection) {
					Console::ExecuteQuestCommandWithMessageBox("movetoqt", selection[0].GetFormID());
					Menu::GetSingleton()->Close();
				}
			}

			ImGui::Separator();

			if (ImGui::Button("Complete Quest", ImVec2(-1, 0))) {
				if (hasSelection) {
					Console::ExecuteQuestCommandWithMessageBox("completequest", selection[0].GetFormID());
					Menu::GetSingleton()->Close();
				}
			}

			if (ImGui::Button("Reset Quest", ImVec2(-1, 0))) {
				if (hasSelection) {
					Console::ExecuteQuestCommandWithMessageBox("resetquest", selection[0].GetFormID());
					Menu::GetSingleton()->Close();
				}
			}

			ImGui::Separator();

			static int stageValue = 10;
			ImGui::SetNextItemWidth(-1);
			ImGui::InputInt("Stage Value", &stageValue, 5);

			if (ImGui::Button("Set Stage", ImVec2(-1, 0))) {
				if (hasSelection) {
					Console::ExecuteQuestCommandWithMessageBox("setstage", selection[0].GetFormID(), stageValue);
					Menu::GetSingleton()->Close();
				}
			}

			ImGui::EndDisabled();
		}

		ImGui::PopStyleVar();
	}

	void QuestWindow::ShowQuestContextMenu(const QuestData& quest)
	{
		if (ImGui::BeginMenu("Quest Commands")) {
			if (ImGui::MenuItem("Show Quest Targets")) {
				Console::ExecuteQuestCommandWithMessageBox("showquesttargets");
			}

			if (ImGui::MenuItem("Show Quest Objectives")) {
				Console::ExecuteQuestCommandWithMessageBox("sqo");
			}

			if (ImGui::MenuItem("Get Stage")) {
				Console::ExecuteQuestCommandWithMessageBox("getstage", quest.GetFormID());
			}

			if (ImGui::MenuItem("Show Quest Stages")) {
				Console::ExecuteQuestCommandWithMessageBox("sqs", quest.GetFormID());
			}

			if (ImGui::MenuItem("Show Quest Variables")) {
				Console::ExecuteQuestCommandWithMessageBox("sqv", quest.GetFormID());
			}

			if (ImGui::MenuItem("Move To Quest Target")) {
				Console::ExecuteQuestCommandWithMessageBox("movetoqt", quest.GetFormID());
				Menu::GetSingleton()->Close();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Complete Quest")) {
				Console::ExecuteQuestCommandWithMessageBox("completequest", quest.GetFormID());
				Menu::GetSingleton()->Close();
			}

			if (ImGui::MenuItem("Reset Quest")) {
				Console::ExecuteQuestCommandWithMessageBox("resetquest", quest.GetFormID());
				Menu::GetSingleton()->Close();
			}

			ImGui::Separator();

			static int stageValue = 10;
			ImGui::SetNextItemWidth(100);
			ImGui::InputInt("Stage", &stageValue, 5);

			if (ImGui::MenuItem("Set Stage")) {
				Console::ExecuteQuestCommandWithMessageBox("setstage", quest.GetFormID(), stageValue);
				Menu::GetSingleton()->Close();
			}

			ImGui::EndMenu();
		}
	}

	void QuestWindow::Init(bool is_default)
	{
		b_ClickToAdd = true;
		clickToAddCount = 1;
		activeViewport = Viewport::TableView;

		questTableView.SetGenerator([]() { return Data::GetSingleton()->GetQuestList(); });
		questTableView.SetupSearch(Data::PLUGIN_TYPE::QUEST);
		questTableView.SetClickAmount(&clickToAddCount);
		questTableView.SetDoubleClickBehavior(&b_ClickToAdd);
		questTableView.Init();

		if (is_default) {
			questTableView.Refresh();
			questTableView.BuildPluginList();
		}
	}

	void QuestWindow::Unload()
	{
		questTableView.Unload();
		activeQuestList.clear();
		activeQuestSelection.reset();
	}

	void QuestWindow::Load()
	{
		questTableView.Load();
		UpdateActiveQuestList();
	}

	void QuestWindow::Refresh()
	{
		// Make sure the quest data is generated first
		Data::GetSingleton()->GenerateQuestList();

		// Then refresh the table view
		questTableView.Refresh();

		// Update the active quest list
		UpdateActiveQuestList();

		// Log the number of quests found for debugging
		logger::info("QuestWindow::Refresh - Loaded {} quests", questTableView.GetTableList().size());
	}

	void QuestWindow::BuildPluginList()
	{
		questTableView.BuildPluginList();
	}

	void QuestWindow::UpdateActiveQuestList()
	{
		activeQuestList.clear();

		// Get all quests directly from Data
		auto allQuests = Data::GetSingleton()->GetQuestList();

		// Filter for active quests
		for (const auto& quest : allQuests) {
			if (quest.IsActive() || quest.IsStarting()) {
				activeQuestList.push_back(quest);
			}
		}

		logger::info("QuestWindow::UpdateActiveQuestList - Found {} active quests", activeQuestList.size());
	}
}
