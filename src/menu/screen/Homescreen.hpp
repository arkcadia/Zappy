/**
 * @file Homescreen.hpp
 * @brief Default landing page with animated hero and action buttons.
 *
 * @details HomeScreen is the first page shown on launch and the destination
 * of any "go home" navigation.  It fills the content area to the right of
 * the Sidebar and between TitleBar and BottomBar.
 *
 * @par Visual composition
 * ```
 * ┌──────────────────────────────────────────────────────────┐
 * │  ░░░░ grid ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  ┌──────────┐ │
 * │  · · animated · · particles · · · · · · ·  │ NEWS     │ │
 * │                                            │ CARD     │ │
 * │  ZAPPY                                     │          │ │
 * │  WORLD                                     └──────────┘ │
 * │  [LAUNCH ALL]  [S]                                       │
 * └──────────────────────────────────────────────────────────┘
 * ```
 *
 * @par Animation
 * The hero background animates via `m_animTime` (accumulated from
 * `GetTime()`).  Nine particles drift vertically with a `sin`-based
 * offset and pulse in opacity, giving a subtle living-background effect
 * without any external asset dependency.
 *
 * @par Interaction
 * - **LAUNCH ALL** button → `state.openOverlay(Overlay::Settings)`
 * - **S** icon button    → `state.openOverlay(Overlay::Settings)`
 * - News card **×**      → `m_newsVisible = false`
 * - News card **▶**      → hover highlight only (no action yet)
 * - News card link       → hover highlight only (no action yet)
 *
 * @ingroup Screens
 */

#pragma once
#include "../ui/Widgets.hpp"
#include "screen.hpp"

namespace zappy {

/**
 * @brief Landing page implementing the Screen interface.
 *
 * Inherits from Screen and is stored by value in Application.
 * All geometry is computed each frame from the live screen dimensions
 * so the layout responds correctly to window resizing.
 *
 * @ingroup Screens
 */
class HomeScreen : public Screen {
  public:
    /**
     * @brief Construct the HomeScreen and initialise its action buttons.
     *
     * Sets up `m_launchBtn` with the default primary ButtonStyle and label
     * "LAUNCH ALL", and `m_settingsBtn` with the icon ButtonStyle and label "S".
     */
    HomeScreen();

    /**
     * @brief Update button state and handle news-card dismiss.
     *
     * Recomputes button bounds from the live window dimensions each frame.
     * Checks the news-card close hit-box when `m_newsVisible` is true.
     * Opens the Settings overlay when either action button is clicked.
     *
     * @param state Mutable application state; may call openOverlay(Settings).
     */
    void update(AppState &state) override;

    /**
     * @brief Render the hero background, headings, news card, and buttons.
     *
     * Draw order:
     * 1. drawHeroBackground() – grid, particles, edge gradients.
     * 2. drawHeroText()       – "ZAPPY / WORLD" heading.
     * 3. drawNewsCard()       – dismissible news card (if visible).
     * 4. `m_launchBtn.draw()` and `m_settingsBtn.draw()`.
     * 5. "S" icon label over the settings button.
     *
     * @param state Read-only application state (unused by HomeScreen itself).
     */
    void draw(const AppState &state) override;

  private:
    ui::Button m_launchBtn;    ///< Primary "LAUNCH ALL" call-to-action button.
    ui::Button m_settingsBtn;  ///< Small "S" icon shortcut to Settings overlay.
    bool m_newsVisible = true; ///< Controls visibility of the news card.
    float m_animTime = 0.0f;   ///< Accumulated time in seconds (from GetTime()) for animation.

    /**
     * @brief Draw the animated hero area background.
     *
     * Renders:
     * - Dark base rectangle `{14, 10, 24, 255}`.
     * - Purple grid lines (every 50 px, 10/255 alpha).
     * - Nine drifting, pulsing particle dots.
     * - Left-edge fade gradient blending into the sidebar colour.
     * - Bottom-edge fade gradient blending into the app background.
     *
     * @param x Left edge of the hero area (== Sidebar::WIDTH).
     * @param y Top edge of the hero area (== TitleBar::HEIGHT).
     * @param w Width of the hero area.
     * @param h Height of the hero area.
     */
    void drawHeroBackground(float x, float y, float w, float h) const;

    /**
     * @brief Draw the "ZAPPY" / "WORLD" stacked heading.
     *
     * Uses the monospace font at 48 px with 2 px letter-spacing, in white,
     * positioned in the lower-left of the hero area.
     *
     * @param x  Left edge of the text block (== Sidebar::WIDTH + 32).
     * @param baseY Bottom anchor of the hero area (titleH + heroH).
     */
    void drawHeroText(float x, float baseY) const;

    /**
     * @brief Draw the dismissible "LATEST NEWS" card.
     *
     * The card contains:
     * - Rounded dark background with accent border.
     * - "LATEST NEWS" sub-label and dismiss "x" button.
     * - Thumbnail strip with ellipse decorations and a play-button circle.
     * - Body text "Server v1.0 – ready to launch".
     * - "Configure & Launch >>" link (highlights on hover).
     *
     * @param x Left edge of the card.
     * @param y Top edge of the card.
     */
    void drawNewsCard(float x, float y) const;

    /**
     * @brief Reserved for a decorative tower graphic element.
     *
     * @note Not yet implemented.
     *
     * @param x Left edge.
     * @param y Top edge.
     * @param h Height.
     */
    void drawTower(float x, float y, float h) const;
};

} // namespace zappy