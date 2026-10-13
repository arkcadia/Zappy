/**
 * @file SettingsPanel.hpp
 * @brief Scrollable launch-configuration overlay with live CLI preview.
 *
 * @details SettingsPanel is displayed as a full content-area overlay when
 * `AppState::currentOverlay == Overlay::Settings`.  It presents the launch
 * configuration fields grouped into three collapsible-style sections, plus a
 * live command-preview box at the bottom that renders the assembled CLI
 * strings in real time.
 *
 * @par Field inventory
 * | Section    | Field         | ConfigManager path       | CLI flag |
 * |------------|---------------|--------------------------|----------|
 * | SERVER     | Port          | server.port              | `-p`     |
 * | SERVER     | Map width     | server.width             | `-x`     |
 * | SERVER     | Map height    | server.height            | `-y`     |
 * | SERVER     | Frequency     | server.freq              | `-f`     |
 * | SERVER     | Max clients   | server.clients           | `-c`     |
 * | SERVER     | Teams         | server.teams (space-sep) | `-n`     |
 * | AI CLIENT  | Host          | ai.host                  | `-h`     |
 * | AI CLIENT  | Team counts   | ai.teamCounts            | `-n`     |
 * | GUI        | Host          | gui.host                 | `-h`     |
 *
 * @par Config synchronisation
 * ```
 * Constructor / reopen ──► syncFromConfig()  ← ConfigManager → fields
 * Every update() frame  ──► syncToConfig()   ← fields → ConfigManager
 * ```
 * This means ConfigManager always reflects the current field state,
 * and the command-preview box (which calls ConfigManager::build*Cmd())
 * updates live as the user types.
 *
 * @par Scroll behaviour
 * The form content is approximately 820 px tall.  A ScrollView clips and
 * translates the content area; a thin purple thumb appears on the right
 * edge when scrolling is possible.
 *
 * @par Dismiss
 * - **LAUNCH** button (bottom-right) – opens the launch-sequence modal.
 * - **CANCEL** button – closes overlay, navigates to Home.
 * - **× icon** (top-right corner of panel) – same as CANCEL.
 *
 * @ingroup Screens
 */

#pragma once
#include "../app/AppState.hpp"
#include "../ui/Widgets.hpp"
#include <vector>

namespace zappy {

/**
 * @brief Scrollable launch-configuration form overlay.
 *
 * Does **not** inherit from Screen because it is an overlay, not a
 * full-page replacement.  Owned by Application and only updated/drawn
 * when AppState::isOverlay(Overlay::Settings) is true.
 *
 * @ingroup Screens
 */
class SettingsPanel {
  public:
    /**
     * @brief Construct the panel and initialise all text input fields.
     *
     * Configures each TextInput with its label, flag, numeric mode, and
     * font size.  Sets up the footer buttons.
     * Calls syncFromConfig() to pre-populate fields from the current config.
     */
    SettingsPanel();

    /**
     * @brief Populate all text fields from the persisted ConfigManager state.
     *
     * Converts integer config values to strings; joins the teams vector with
     * spaces.  Should be called whenever the panel is (re-)opened so that
     * any external config changes (e.g. load from disk) are reflected.
     */
    void syncFromConfig();

    /**
     * @brief Write all field values back into ConfigManager.
     *
     * Parses numeric fields with a fallback-to-default on failure.
     * Splits the teams string on whitespace; ensures at least one team.
     * Applies fallback hostnames ("localhost") and team names ("Team1")
     * when the corresponding fields are empty.
     *
     * Called every frame in update() so that ConfigManager::build*Cmd()
     * always reflects the current UI state.
     */
    void syncToConfig();

    /**
     * @brief Handle scroll, focus clicks, keyboard input, and dismiss buttons.
     *
     * Per-frame responsibilities:
     * 1. Update ScrollView from mouse-wheel input.
     * 2. Recompute all TextInput::bounds and per-team AI controls.
     * 3. Recompute LAUNCH, CANCEL, and close-button bounds.
     * 4. Dispatch handleFocusClick() for left-click focus management.
     * 5. Call TextInput::update() on the focused field only.
     * 6. Call syncToConfig() to keep ConfigManager in sync.
     * 7. Handle LAUNCH button click → openOverlay(LaunchModal).
     * 8. Handle CANCEL button click → closeOverlay() + navigate(Home).
     * 9. Handle top-right × click → same as CANCEL.
     *
     * @param state Mutable application state; may call closeOverlay() and navigate(Home).
     */
    void update(AppState &state);

    /**
     * @brief Render the panel background, scrollable form, and footer.
     *
     * Draw order:
     * 1. Solid near-opaque overlay background `{8, 6, 16, 248}`.
     * 2. ScrollView clip region containing drawSettingsContent().
     * 3. Footer strip (separator + LAUNCH/CANCEL buttons + × button), drawn
     *    outside the clip so it is always visible at the bottom.
     *
     * @param state Unused (const for Application call-site symmetry).
     */
    void draw(const AppState &state) const;

  private:
    struct AiTeamRow {
        std::string team;       ///< Server team name represented by the row.
        ui::TextInput count;    ///< Number of AI clients for this team.
        ui::Button minusButton; ///< Decrease team AI count.
        ui::Button plusButton;  ///< Increase team AI count.
    };

    // ── Server section ─────────────────────────────────────────────────────
    ui::TextInput m_serverPort;    ///< Server TCP port (`-p`), numeric.
    ui::TextInput m_serverWidth;   ///< Map width in tiles (`-x`), numeric.
    ui::TextInput m_serverHeight;  ///< Map height in tiles (`-y`), numeric.
    ui::TextInput m_serverFreq;    ///< Simulation frequency in Hz (`-f`), numeric.
    ui::TextInput m_serverClients; ///< Max clients per team (`-c`), numeric.
    ui::TextInput m_serverTeams;   ///< Space-separated team names (`-n`), text.

    // ── AI section ─────────────────────────────────────────────────────────
    ui::TextInput m_aiHost; ///< AI client hostname (`-h`), text.
    ui::TextInput m_aiPort; ///< AI client port (`-p`), numeric.
    std::vector<AiTeamRow> m_aiRows; ///< Per-team AI client count controls.

    // ── GUI section ────────────────────────────────────────────────────────
    ui::TextInput m_guiHost; ///< GUI client hostname (`-h`), text.
    ui::TextInput m_guiPort; ///< GUI client port (`-p`), numeric.
    ui::TextInput m_guiFrontend; ///< GUI frontend selector (`2d` or `3d`).

    // ── Chrome ─────────────────────────────────────────────────────────────
    ui::Button m_gui2dBtn;    ///< Frontend selector for the 2D GUI.
    ui::Button m_gui3dBtn;    ///< Frontend selector for the 3D GUI.
    ui::Button m_launchBtn;  ///< "LAUNCH" primary button in the footer.
    ui::Button m_cancelBtn;  ///< "CANCEL" ghost button in the footer.
    ui::ScrollView m_scroll; ///< Scroll container for the form content area.
    ui::TextInput *m_focusedInput =
        nullptr; ///< Pointer to the currently focused field, or nullptr.

    /**
     * @brief Draw the scrollable form content (headings, sections, cmd preview).
     *
     * All geometry is expressed in already-scrolled screen coordinates.
     *
     * @param x      Inner left edge of the content column.
     * @param y      Top of the scrolled coordinate space.
     * @param w      Inner width of the content column.
     * @param scrollY Unused legacy parameter (kept for call-site compatibility).
     */
    void drawSettingsContent(float x, float y, float w, float scrollY) const;

    /**
     * @brief Set `input.bounds` and delegate to `input.draw()`.
     *
     * Convenience wrapper used in the draw path to avoid repeating the
     * bounds-assignment boilerplate.
     *
     * @param input Reference to the TextInput to position and draw.
     * @param x     Left edge of the field.
     * @param y     Top edge of the field.
     * @param w     Width of the field.
     */
    void drawInputField(ui::TextInput &input, float x, float y, float w) const;

    /**
     * @brief Recompute TextInput::bounds for all fields from the current layout.
     *
     * @note Currently unused.  Reserved for future refactoring.
     *
     * @param x Unused.
     * @param y Unused.
     * @param w Unused.
     */
    void updateInputPositions(float x, float y, float w);

    /**
     * @brief Parse the server team text field into a non-empty list.
     * @return Team names from the field, or `Team1` as a fallback.
     */
    std::vector<std::string> parseServerTeams() const;

    /**
     * @brief Ensure the AI count controls match the current server teams.
     *
     * Existing row values are preserved by team name. Missing teams are
     * initialised from ConfigManager, then fall back to zero clients.
     *
     * @param teams Current server team list.
     * @return true if the row list was rebuilt.
     */
    bool ensureAiRowsForTeams(const std::vector<std::string> &teams);

    /**
     * @brief Recompute bounds for every visible field and per-team control.
     */
    void layoutFields(float x, float y, float w);

    /**
     * @brief Transfer keyboard focus to whichever field was clicked.
     *
     * On a left mouse button press, iterates all visible text fields.  Sets
     * `focused = true` for the field under the cursor and `false` for all
     * others.  Updates `m_focusedInput` accordingly (nullptr if no field hit).
     *
     * Only reacts on `IsMouseButtonPressed(MOUSE_BUTTON_LEFT)`; no-op otherwise.
     */
    void handleFocusClick();

    // ── Stub private draw helpers (reserved for future section decomposition) ──

    /// @private Reserved for extracting the server section draw into its own method.
    void drawSectionServer(float x, float &y, float w) const;

    /// @private Reserved for extracting the AI section draw into its own method.
    void drawSectionAi(float x, float &y, float w) const;

    /// @private Reserved for extracting the GUI section draw into its own method.
    void drawSectionGui(float x, float &y, float w) const;

    /// @private Reserved for extracting the command-preview draw into its own method.
    void drawCmdPreviewSection(float x, float &y, float w) const;
};

} // namespace zappy
