// Included once by each host's Menu.cpp after its native widget definitions.
namespace Ship {
void Menu::DrawPopoutControls(const std::string& section, const std::string& sidebar,
                             const std::set<std::string>& cvars) {
    const auto header = menuEntries.find(section);
    if (header == menuEntries.end())
        return;
    const auto entry = header->second.sidebars.find(sidebar);
    if (entry == header->second.sidebars.end())
        return;
    for (auto& [reason, info] : disabledMap)
        info.active = info.evaluation(info);
#ifdef COMBO_MENU_POPOUT_OOT
    raceDisableActive = CVarGetInteger(CVAR_SETTING("DisableChanges"), 0);
#endif
    menuThemeIndex = static_cast<UIWidgets::Colors>(CVarGetInteger("gSettings.Menu.Theme", defaultThemeIndex));
    std::vector<std::vector<WidgetInfo*>> columns;
    for (auto& column : entry->second.columnWidgets) {
        std::vector<WidgetInfo*> selected;
        for (auto& widget : column) {
            if (widget.type != WIDGET_WINDOW_BUTTON &&
                (cvars.empty() || (widget.cVar && cvars.count(widget.cVar))))
                selected.push_back(&widget);
        }
        if (!selected.empty())
            columns.push_back(std::move(selected));
    }
    if (columns.empty())
        return;
    ImGui::PushID(this);
    if (ImGui::BeginTable("Detached controls", static_cast<int>(columns.size()),
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame)) {
        for (auto& column : columns) {
            ImGui::TableNextColumn();
            for (auto* widget : column) {
                ImGui::PushID(widget);
#ifdef COMBO_MENU_POPOUT_OOT
                MenuDrawItem(*widget, menuThemeIndex);
#else
                MenuDrawItem(*widget, 90 / columns.size(), menuThemeIndex);
#endif
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}
} // namespace Ship
