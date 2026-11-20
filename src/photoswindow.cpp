// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "photoswindow.h"
#include "globalmenu.h"
#include <base/paths.h>
#include <base/i18n.h>
#include <ui/about_dialog.h>
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
#include <cstdlib>

namespace Photos {
using namespace Lingmo::ui;
using Lingmo::I18n::tr;
namespace {
class Root final : public View {
public:
    Root(PhotosWindow& owner, PhotoStore& store, LibraryScanner& scanner) : m_owner(owner), m_store(store), m_scanner(scanner) {
        m_sidebar = addChild<SidebarList>(std::vector<SidebarList::Entry>{{"photos", tr("Photos"), "image-x-generic"}, {"recent", tr("Recently Added"), "document-open-recent"}, {"favorites", tr("Favorites"), "starred"}, {"albums", tr("Albums"), "folder"}, {"folders", tr("Folders"), "folder-open"}, {"trash", tr("Recently Deleted"), "user-trash"}});
        m_sidebar->onActivated = [this](const std::string& id) { m_page = id; refresh(); };
        m_title = addChild<Label>(tr("Photos")); m_title->setFontSize(24); m_title->setWeight(600);
        m_search = addChild<SearchField>(); m_search->setPlaceholder(tr("Search photos")); m_search->onTextChanged = [this] { refresh(); };
        m_import = addChild<Button>(tr("Import")); m_import->onClicked = [this] { scanPictures(); };
        m_scroll = addChild<ScrollView>(); m_grid = m_scroll->setContent<Grid>(5); m_grid->setColumnSpacing(12); m_grid->setRowSpacing(12); m_grid->setMargins({28, 16, 28, 24});
        m_status = addChild<Label>(); m_status->setColor(ColorId::DisabledText);
        m_subscriptions.push_back(m_scanner.changed.connect([this] { refresh(); }));
        m_page = "photos"; refresh();
    }
    void layout() override {
        const double side = 220; m_sidebar->setGeometry({0, 46, side, std::max(0.0, geometry().height - 46)}); m_title->setGeometry({side + 28, 20, 280, 34}); m_search->setGeometry({std::max(side + 320.0, geometry().width - 360), 18, 240, 36}); m_import->setGeometry({std::max(side + 570.0, geometry().width - 110), 18, 82, 36}); m_scroll->setGeometry({side, 70, std::max(0.0, geometry().width - side), std::max(0.0, geometry().height - 106)}); m_status->setGeometry({side + 28, std::max(0.0, geometry().height - 30), 300, 24});
    }
    void paint(Painter& painter) override { const auto& theme = Theme::current(); painter.fillRect(localRect(), theme.background); painter.fillRect({0, 0, 220, geometry().height}, theme.secondBackground); painter.fillRect({220, 0, 1, geometry().height}, theme.text.withAlpha(.08)); }
private:
    void refresh() {
        while (!m_grid->children().empty()) m_grid->takeChild(m_grid->children().front().get());
        auto photos = m_search->text().empty() ? (m_page == "favorites" ? m_store.photos(true) : m_page == "trash" ? m_store.photos(false, true) : m_store.photos()) : m_store.search(m_search->text());
        for (const auto& photo : photos) { auto* tile = m_grid->addChild<Column>(); tile->setSpacing(4); auto* image = tile->addChild<ImageView>(photo.path); image->setThumbnail(true); image->setAutoTransform(true); image->setFillMode(ImageView::FillMode::PreserveAspectCrop); image->setCornerRadius(10); image->setLayoutHints({.preferredWidth = 150, .preferredHeight = 112}); auto* name = tile->addChild<Label>(photo.name); name->setElide(Label::Elide::Middle); name->setLayoutHints({.fillWidth = true}); }
        m_status->setText(std::to_string(photos.size()) + " " + tr("photos")); requestLayout(); requestPaint();
    }
    void scanPictures() { const char* home = std::getenv("HOME"); if (home) m_scanner.scan(std::string(home) + "/Pictures"); }
    PhotosWindow& m_owner; PhotoStore& m_store; LibraryScanner& m_scanner; SidebarList* m_sidebar = nullptr; Label* m_title = nullptr; SearchField* m_search = nullptr; Button* m_import = nullptr; ScrollView* m_scroll = nullptr; Grid* m_grid = nullptr; Label* m_status = nullptr; std::string m_page; std::vector<base::Subscription> m_subscriptions;
};
}
PhotosWindow::PhotosWindow(std::string database) : m_store(std::move(database)), m_scanner(m_store) {
    m_window = std::make_unique<Window>(); m_window->setTitle(tr("Photos")); m_window->setMinimumSize({860, 560}); m_window->resize({1120, 720});
    m_about = std::make_unique<AboutDialog>(m_window.get()); m_about->setName(tr("Photos")); m_about->setIconSource("lingmo-photos"); m_about->setDescription(tr("Manage, browse and edit your photo library."));
    m_menu = std::make_unique<GlobalMenu>(Application::instance()->sessionBus(), [this](int32_t id) { if (id == 2) { const char* home = std::getenv("HOME"); if (home) m_scanner.scan(std::string(home) + "/Pictures"); } else if (id == 3) Application::instance()->quit(); else if (id == 5) showAbout(); });
    m_root = new Root(*this, m_store, m_scanner); m_window->setContentView(std::unique_ptr<View>(m_root));
    const char* home = std::getenv("HOME"); if (home) m_scanner.scan(std::string(home) + "/Pictures");
}
PhotosWindow::~PhotosWindow() = default;
void PhotosWindow::show() { m_window->show(); }
void PhotosWindow::showAbout() { m_about->show(); }
Window& PhotosWindow::window() { return *m_window; }
void PhotosWindow::refresh() { if (m_root) m_root->requestLayout(); }
} // namespace Photos
