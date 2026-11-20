// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "photoswindow.h"
#include "globalmenu.h"
#include <base/paths.h>
#include <base/i18n.h>
#include <base/event_loop.h>
#include <ui/about_dialog.h>
#include <ui/animation.h>
#include <ui/application.h>
#include <ui/button.h>
#include <ui/image_view.h>
#include <ui/label.h>
#include <ui/layout.h>
#include <ui/search_field.h>
#include <ui/scroll_view.h>
#include <ui/sidebar.h>
#include <ui/theme.h>
#include <ui/window.h>
#include <ui/window_header.h>
#include <cstdlib>

namespace Photos {
using namespace Lingmo::ui;
using Lingmo::I18n::tr;
namespace {
class PhotoTile final : public View {
public:
    PhotoTile(Photo photo, std::function<void()> open)
        : m_photo(std::move(photo)), m_open(std::move(open)) {
        setTracksHover(true); setCursorShape(CursorShape::Pointer);
        m_image = addChild<ImageView>(m_photo.path); m_image->setThumbnail(true); m_image->setAutoTransform(true); m_image->setFillMode(ImageView::FillMode::PreserveAspectCrop); m_image->setCornerRadius(10);
        m_label = addChild<Label>(m_photo.name); m_label->setElide(Label::Elide::Middle);
    }
    SizeF sizeHint() const override { return {150, 140}; }
    bool acceptsPointer() const override { return true; }
protected:
    void layout() override { m_image->setGeometry({0, 0, geometry().width, 112}); m_label->setGeometry({0, 116, geometry().width, 22}); }
    void paint(Painter& painter) override {
        const auto& theme = Theme::current();
        const auto fill = isHovered() ? theme.text.withAlpha(theme.darkMode ? .12 : .07) : theme.text.withAlpha(theme.darkMode ? .07 : .035);
        painter.drawSmoothRectangle(localRect(), theme.mediumRadius, fill);
    }
    void pointerRelease(const PointerEvent& event) override { if (event.button == MouseButton::Left && m_open) m_open(); }
private:
    Photo m_photo; std::function<void()> m_open; ImageView* m_image = nullptr; Label* m_label = nullptr;
};

class Viewer final : public View {
public:
    Viewer(std::function<void()> close, std::function<void()> previous, std::function<void()> next,
           std::function<void()> favorite, std::function<void()> trash,
           std::function<void(bool)> fullScreen, std::function<void(bool)> slideShow, std::function<void()> restore)
        : m_close(std::move(close)), m_previous(std::move(previous)), m_next(std::move(next)), m_favorite(std::move(favorite)), m_trash(std::move(trash)), m_fullScreen(std::move(fullScreen)), m_slideShow(std::move(slideShow)), m_restore(std::move(restore)) {
        m_back = addChild<Button>(tr("Back")); m_back->onClicked = m_close;
        m_prev = addChild<Button>(tr("Previous")); m_prev->onClicked = m_previous;
        m_nextButton = addChild<Button>(tr("Next")); m_nextButton->onClicked = m_next;
        m_favoriteButton = addChild<Button>(tr("Favorite")); m_favoriteButton->onClicked = m_favorite;
        m_trashButton = addChild<Button>(tr("Move to Trash")); m_trashButton->onClicked = m_trash;
        m_zoomOut = addChild<Button>(tr("Zoom Out")); m_zoomOut->onClicked = [this] { setZoom(m_zoom - .15); };
        m_zoomIn = addChild<Button>(tr("Zoom In")); m_zoomIn->onClicked = [this] { setZoom(m_zoom + .15); };
        m_resetZoom = addChild<Button>(tr("Reset")); m_resetZoom->onClicked = [this] { setZoom(1); };
        m_flip = addChild<Button>(tr("Flip")); m_flip->onClicked = [this] { m_flipped = !m_flipped; updateTransform(); };
        m_fullScreenButton = addChild<Button>(tr("Full Screen")); m_fullScreenButton->onClicked = [this] { m_fullScreen(!m_fullScreenState); m_fullScreenState = !m_fullScreenState; };
        m_slideButton = addChild<Button>(tr("Slideshow")); m_slideButton->onClicked = [this] { m_sliding = !m_sliding; m_slideShow(m_sliding); m_slideButton->setText(m_sliding ? tr("Stop Slideshow") : tr("Slideshow")); };
        m_restoreButton = addChild<Button>(tr("Restore")); m_restoreButton->onClicked = m_restore;
        m_image = addChild<ImageView>(); m_image->setFillMode(ImageView::FillMode::PreserveAspectFit); m_image->setAutoTransform(true);
        m_info = addChild<Label>(); m_info->setColor(ColorId::DisabledText); m_info->setWrap(true);
    }
    void setPhoto(const Photo& photo, size_t index, size_t count) {
        m_image->setSource(photo.path); m_image->setSourceSize({geometry().width * .72, geometry().height * .72}); m_info->setText(photo.name + "\n" + photo.folder + "\n" + photo.captured + "\n" + photo.format + "  " + std::to_string(photo.bytes) + " bytes"); m_prev->setEnabled(index > 0); m_nextButton->setEnabled(index + 1 < count); m_favoriteButton->setText(photo.favorite ? tr("Unfavorite") : tr("Favorite")); m_trashButton->setText(photo.trashed ? tr("Delete Permanently") : tr("Move to Trash")); m_restoreButton->setVisible(photo.trashed); m_trashButton->setVisible(!photo.trashed); m_zoom = 1; m_flipped = false; updateTransform();
    }
protected:
    void layout() override {
        m_back->setGeometry({24, 18, 80, 36}); m_prev->setGeometry({120, 18, 76, 36}); m_nextButton->setGeometry({200, 18, 76, 36}); m_zoomOut->setGeometry({300, 18, 82, 36}); m_zoomIn->setGeometry({388, 18, 82, 36}); m_resetZoom->setGeometry({476, 18, 72, 36}); m_flip->setGeometry({556, 18, 72, 36}); m_fullScreenButton->setGeometry({636, 18, 106, 36}); m_slideButton->setGeometry({750, 18, 110, 36}); m_restoreButton->setGeometry({geometry().width - 370, 18, 110, 36}); m_favoriteButton->setGeometry({geometry().width - 250, 18, 100, 36}); m_trashButton->setGeometry({geometry().width - 140, 18, 116, 36}); m_image->setGeometry({24, 72, std::max(0.0, geometry().width - 300), std::max(0.0, geometry().height - 110)}); m_info->setGeometry({geometry().width - 250, 80, 220, 180});
    }
    void paint(Painter& painter) override { painter.fillRect(localRect(), Theme::current().background); }
private:
    void setZoom(double value) { m_zoom = std::clamp(value, .5, 3.0); updateTransform(); }
    void updateTransform() { if (m_image) m_image->setTransform({{}, (m_flipped ? -1 : 1) * m_zoom}); }
    std::function<void()> m_close, m_previous, m_next, m_favorite, m_trash, m_restore; std::function<void(bool)> m_fullScreen, m_slideShow; Button *m_back, *m_prev, *m_nextButton, *m_favoriteButton, *m_trashButton, *m_zoomOut, *m_zoomIn, *m_resetZoom, *m_flip, *m_fullScreenButton, *m_slideButton, *m_restoreButton; ImageView* m_image; Label* m_info; double m_zoom = 1; bool m_flipped = false, m_fullScreenState = false, m_sliding = false;
};

class Root final : public View {
public:
    Root(PhotosWindow& owner, PhotoStore& store, LibraryScanner& scanner) : m_owner(owner), m_store(store), m_scanner(scanner) {
        m_sidebar = addChild<SidebarList>(std::vector<SidebarList::Entry>{{"photos", tr("Photos"), "image-x-generic"}, {"recent", tr("Recently Added"), "document-open-recent"}, {"favorites", tr("Favorites"), "starred"}, {"albums", tr("Albums"), "folder"}, {"folders", tr("Folders"), "folder-open"}, {"trash", tr("Recently Deleted"), "user-trash"}});
        m_sidebar->onActivated = [this](const std::string& id) { m_page = id; m_headerTitle->setText(id == "photos" ? tr("Photos") : id == "recent" ? tr("Recently Added") : id == "favorites" ? tr("Favorites") : id == "albums" ? tr("Albums") : id == "folders" ? tr("Folders") : tr("Recently Deleted")); refresh(); };
        m_appTitle = addChild<Label>(tr("Photos")); m_appTitle->setFontSize(18); m_appTitle->setWeight(600); m_appTitle->setVerticallyCentered(true);
        m_header = addChild<WindowHeader>(46);
        auto* headerContent = m_header->setContent<Row>();
        auto* sidebarGap = headerContent->addChild<View>(); sidebarGap->setLayoutHints({.preferredWidth = 220});
        m_headerTitle = headerContent->addChild<Label>(); m_headerTitle->setFontSize(17); m_headerTitle->setVerticallyCentered(true); m_headerTitle->setLayoutHints({.fillWidth = true});
        m_header->setMinimizeVisible(true); m_header->setMaximizeVisible(true); m_header->setCloseVisible(true);
        m_search = addChild<SearchField>(); m_search->setPlaceholder(tr("Search photos")); m_search->onTextChanged = [this] { refresh(); };
        m_import = addChild<Button>(tr("Import")); m_import->onClicked = [this] { scanPictures(); };
        m_scroll = addChild<ScrollView>(); m_content = m_scroll->setContent<Column>(); m_content->setSpacing(20); m_content->setMargins({28, 16, 28, 24});
        m_status = addChild<Label>(); m_status->setColor(ColorId::DisabledText);
        m_viewer = addChild<Viewer>([this] { closeViewer(); }, [this] { moveViewer(-1); }, [this] { moveViewer(1); }, [this] { toggleViewerFavorite(); }, [this] { trashViewer(); }, [this](bool enabled) { m_owner.window().setFullScreen(enabled); }, [this](bool enabled) { setSlideShow(enabled); }, [this] { restoreViewer(); }); m_viewer->setVisible(false);
        m_subscriptions.push_back(m_scanner.changed.connect([this] { refresh(); }));
        m_page = "photos"; refresh();
    }
    void layout() override {
        const double side = 220; m_header->setGeometry({0, 0, geometry().width, 46}); m_appTitle->setGeometry({20, 0, side - 40, 46}); m_sidebar->setGeometry({0, 46, side, std::max(0.0, geometry().height - 46)}); m_search->setGeometry({std::max(side + 320.0, geometry().width - 360), 56, 240, 36}); m_import->setGeometry({std::max(side + 570.0, geometry().width - 110), 56, 82, 36}); m_scroll->setVisible(!m_viewer->isVisible()); m_search->setVisible(!m_viewer->isVisible()); m_import->setVisible(!m_viewer->isVisible()); m_status->setVisible(!m_viewer->isVisible()); m_scroll->setGeometry({side, 102, std::max(0.0, geometry().width - side), std::max(0.0, geometry().height - 138)}); m_status->setGeometry({side + 28, std::max(0.0, geometry().height - 30), 300, 24}); m_viewer->setGeometry({side, 46, std::max(0.0, geometry().width - side), std::max(0.0, geometry().height - 46)});
    }
    void paint(Painter& painter) override {
        const auto& theme = Theme::current();
        const bool glass = window() && window()->backgroundGlassAvailable();
        const double side = 220;
        painter.fillRect({side, 0, std::max(0.0, geometry().width - side), geometry().height}, theme.background);
        painter.fillRect({0, 0, side, geometry().height}, theme.secondBackground.withAlpha(glass ? .72 : 1));
        painter.fillRect({side, 0, 1, geometry().height}, theme.text.withAlpha(.08));
    }
private:
    void refresh() {
        while (!m_content->children().empty()) m_content->takeChild(m_content->children().front().get());
        auto photos = m_search->text().empty() ? (m_page == "favorites" ? m_store.photos(true) : m_page == "trash" ? m_store.photos(false, true) : m_page == "folders" ? m_store.folder(std::getenv("HOME") ? std::string(std::getenv("HOME")) + "/Pictures" : std::string()) : m_store.photos()) : m_store.search(m_search->text());
        m_visible = photos;
        std::map<std::string, Grid*> groups;
        for (size_t index = 0; index < photos.size(); ++index) { auto photo = photos[index]; const std::string key = photo.captured.empty() ? tr("Undated") : photo.captured; Grid* group = groups.count(key) ? groups[key] : nullptr; if (!group) { m_content->addChild<Label>(key)->setWeight(600); group = m_content->addChild<Grid>(5); group->setColumnSpacing(12); group->setRowSpacing(12); groups.emplace(key, group); } group->addChild<PhotoTile>(photo, [this, index] { openViewer(index); }); }
        m_status->setText(std::to_string(photos.size()) + " " + tr("photos")); requestLayout(); requestPaint();
    }
    void scanPictures() { const char* home = std::getenv("HOME"); if (home) m_scanner.scan(std::string(home) + "/Pictures"); }
    void openViewer(size_t index) {
        if (index >= m_visible.size()) return;
        m_viewerIndex = index;
        m_viewer->setPhoto(m_visible[index], index, m_visible.size());
        m_viewer->setVisible(true);
        m_viewer->setOpacity(0);
        m_viewer->setTransform({{}, .96});
        requestLayout();
        if (!window()) { m_viewer->setOpacity(1); m_viewer->setTransform({}); return; }
        m_transition = std::make_unique<Animation>(window());
        m_transition->setDuration(std::chrono::milliseconds(220));
        m_transition->setEasing(Animation::Easing::OutCubic);
        m_transition->onUpdate = [this](double value) { m_viewer->setOpacity(value); m_viewer->setTransform({{}, .96 + .04 * value}); };
        m_transition->start();
    }
    void closeViewer() {
        if (!m_viewer->isVisible()) return;
        if (!window()) { m_viewer->setVisible(false); requestLayout(); return; }
        m_transition = std::make_unique<Animation>(window());
        m_transition->setDuration(std::chrono::milliseconds(180));
        m_transition->setEasing(Animation::Easing::InOutCubic);
        m_transition->onUpdate = [this](double value) { m_viewer->setOpacity(1 - value); m_viewer->setTransform({{}, 1 - .04 * value}); };
        m_transition->onFinished = [this] { m_viewer->setVisible(false); m_viewer->setOpacity(1); m_viewer->setTransform({}); requestLayout(); };
        m_transition->start();
    }
    void moveViewer(int delta) { const auto next = int(m_viewerIndex) + delta; if (next >= 0 && next < int(m_visible.size())) openViewer(size_t(next)); }
    void toggleViewerFavorite() { if (m_viewerIndex < m_visible.size()) { m_store.setFavorite(m_visible[m_viewerIndex].id, !m_visible[m_viewerIndex].favorite); refresh(); openViewer(m_viewerIndex); } }
    void trashViewer() { if (m_viewerIndex < m_visible.size()) { if (m_page == "trash") m_store.purge(m_visible[m_viewerIndex].id); else m_store.setTrashed(m_visible[m_viewerIndex].id, true); closeViewer(); refresh(); } }
    void restoreViewer() { if (m_viewerIndex < m_visible.size()) { m_store.setTrashed(m_visible[m_viewerIndex].id, false); closeViewer(); refresh(); } }
    void setSlideShow(bool enabled) { if (enabled && window()) { m_slideTimer = Application::instance()->loop().startTimer(std::chrono::seconds(5), true, [this] { moveViewer(1); }); } else if (m_slideTimer) { Application::instance()->loop().stopTimer(m_slideTimer); m_slideTimer = 0; } }
    PhotosWindow& m_owner; PhotoStore& m_store; LibraryScanner& m_scanner; SidebarList* m_sidebar = nullptr; Label* m_appTitle = nullptr; WindowHeader* m_header = nullptr; Label* m_headerTitle = nullptr; SearchField* m_search = nullptr; Button* m_import = nullptr; ScrollView* m_scroll = nullptr; Column* m_content = nullptr; Label* m_status = nullptr; Viewer* m_viewer = nullptr; std::unique_ptr<Animation> m_transition; std::vector<Photo> m_visible; size_t m_viewerIndex = 0; int m_slideTimer = 0; std::string m_page; std::vector<base::Subscription> m_subscriptions;
};
}
PhotosWindow::PhotosWindow(std::string database) : m_store(std::move(database)), m_scanner(m_store) {
    m_window = std::make_unique<Window>(); m_window->setTitle(tr("Photos")); m_window->setMinimumSize({860, 560}); m_window->resize({1120, 720});
    m_about = std::make_unique<AboutDialog>(m_window.get()); m_about->setName(tr("Photos")); m_about->setIconSource("lingmo-photos"); m_about->setDescription(tr("Manage, browse and edit your photo library."));
    m_menu = std::make_unique<GlobalMenu>(Application::instance()->sessionBus(), [this](int32_t id) { if (id == 2) { const char* home = std::getenv("HOME"); if (home) m_scanner.scan(std::string(home) + "/Pictures"); } else if (id == 3) Application::instance()->quit(); else if (id == 5) showAbout(); });
    m_root = new Root(*this, m_store, m_scanner); m_window->setContentView(std::unique_ptr<View>(m_root));
    if (m_window->backgroundGlassAvailable()) {
        m_window->setBackgroundColor(Color{0, 0, 0, 0});
        GlassMaterial glass;
        glass.blurSigma = 20;
        if (Theme::current().darkMode) { glass.tintRed = glass.tintGreen = 0.18; glass.tintBlue = 0.20; }
        m_window->setBackgroundGlass(glass);
    } else {
        m_window->setBackgroundGlass(std::nullopt);
        m_window->setBackgroundColor(Theme::current().background);
    }
    m_window->setBackgroundBlur({0, 0, 220, 720}, Theme::current().windowRadius);
    const char* home = std::getenv("HOME"); if (home) m_scanner.scan(std::string(home) + "/Pictures");
}
PhotosWindow::~PhotosWindow() = default;
void PhotosWindow::show() { m_window->show(); }
void PhotosWindow::showAbout() { m_about->show(); }
Window& PhotosWindow::window() { return *m_window; }
void PhotosWindow::refresh() { if (m_root) m_root->requestLayout(); }
} // namespace Photos
