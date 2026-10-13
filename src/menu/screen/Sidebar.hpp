/**
 * @file Sidebar.hpp
 * @brief Fixed left-hand navigation sidebar.
 *
 * @details The Sidebar renders a 220 px wide vertical panel that is always
 * visible, divided into four zones from top to bottom:
 *
 * | Zone           | Content                                     |
 * |----------------|---------------------------------------------|
 * | Logo (86 px)   | Purple icon square + "ZAPPY / LAUNCHER" text |
 * | Navigation     | Four NavItem widgets (Home, Launch, Logs, Config) |
 * | Socials (72 px)| Four icon links (GitHub, Docs, Bugs, Discord) |
 * | User (58 px)   | Avatar circle, username from config, online badge |
 *
 * @par Navigation behaviour
 * - **Home** → navigate(Home), closeOverlay().
 * - **Launch / Config** → navigate(page), openOverlay(Settings).
 * - **Logs** → navigate(Logs), closeOverlay() (future page).
 *
 * @ingroup Screens
 */

#pragma once
#include "../app/AppState.hpp"
#include "../ui/Widgets.hpp"
#include <vector>

namespace zappy {

/**
 * @brief Left-hand navigation panel, always rendered behind page content.
 *
 * Sidebar is owned by Application and updated / drawn on every frame.
 * It never opens overlays directly — it delegates to AppState — so it
 * remains decoupled from the concrete overlay implementations.
 *
 * @ingroup Screens
 */
class Sidebar {
  public:
    /**
     * @brief Construct the sidebar and populate the navigation entries.
     *
     * Builds the four NavEntry items in display order:
     * Home ([H]), Launch ([>]), Logs ([#]), Config ([*]).
     */
    Sidebar();

    /**
     * @brief Update all NavItem widgets and handle navigation clicks.
     *
     * Recomputes each NavItem::bounds based on the live screen height,
     * sets NavItem::active from the current page, then calls
     * NavItem::update().  On click, invokes AppState::navigate() and
     * conditionally AppState::openOverlay(Overlay::Settings).
     *
     * @param state Mutable application state.
     */
    void update(AppState &state);

    /**
     * @brief Render the full sidebar.
     *
     * Draw order:
     * 1. Right-edge border line.
     * 2. Logo block (drawLogo).
     * 3. Horizontal separator below logo.
     * 4. NavItem widgets.
     * 5. Horizontal separator above socials.
     * 6. Social icon row (drawSocials).
     * 7. Horizontal separator above user area.
     * 8. User area (drawUserArea).
     *
     * @param state Read-only state for active-page highlighting.
     */
    void draw(const AppState &state) const;

    /// @brief Fixed pixel width of the sidebar.
    static constexpr float WIDTH = 220.0f;

  private:
    /**
     * @brief Internal pairing of navigation metadata with a ui::NavItem widget.
     */
    struct NavEntry {
        std::string icon;   ///< Short icon string displayed left of the label (e.g. "[H]").
        std::string label;  ///< Upper-case navigation label (e.g. "HOME").
        Page page;          ///< Target page this entry navigates to.
        ui::NavItem widget; ///< Underlying interactive NavItem widget.
    };

    std::vector<NavEntry> m_navItems; ///< Ordered list of sidebar navigation entries.

    /**
     * @brief Draw the logo icon square and "ZAPPY / LAUNCHER" text.
     *
     * The icon square uses a layered glow effect (four expanding rounded
     * rectangles with decreasing opacity) followed by the solid purple fill.
     *
     * @param x Left edge of the logo block.
     * @param y Top edge of the logo block.
     */
    void drawLogo(float x, float y) const;

    /**
     * @brief Draw the user avatar circle, username, and online status badge.
     *
     * The username is read live from ConfigManager::config().username so
     * it always reflects the persisted value.  The avatar area highlights
     * on hover (future: click to open profile settings).
     *
     * @param y      Top edge of the user area strip.
     * @param height Height of the user area strip in pixels.
     */
    void drawUserArea(float y, float height) const;

    /**
     * @brief Draw the four social-link icon buttons (G, D, B, C).
     *
     * Icons highlight to accent2 on hover.  No click handling is
     * currently implemented.
     *
     * @param y Top edge of the socials strip.
     */
    void drawSocials(float y) const;
};

} // namespace zappy