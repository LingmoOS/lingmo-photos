// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "libraryscanner.h"
#include <base/event_loop.h>
#include <base/thread_pool.h>
#include <algorithm>
#include <filesystem>
#include <cctype>

namespace Photos {
namespace {
bool supported(const std::filesystem::path& path) {
    static const std::vector<std::string> formats = {".jpg", ".jpeg", ".png", ".webp", ".avif", ".gif", ".bmp", ".tif", ".tiff", ".heic", ".heif", ".svg"};
    const auto ext = path.extension().string();
    auto lower = ext;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char value) { return char(std::tolower(value)); });
    return std::find(formats.begin(), formats.end(), lower) != formats.end();
}
}
LibraryScanner::LibraryScanner(PhotoStore& store) : m_store(store) {}
LibraryScanner::~LibraryScanner() { cancel(); }
void LibraryScanner::cancel() { m_cancelled->store(true); }
void LibraryScanner::scan(std::string root, std::function<void()> done) {
    cancel();
    m_cancelled = std::make_shared<std::atomic_bool>(false);
    auto cancelled = m_cancelled;
    auto loop = base::EventLoop::current()->handle();
    base::ThreadPool::global().post([this, root = std::move(root), done = std::move(done), cancelled, loop] {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied)) {
                if (cancelled->load()) return;
                if (!entry.is_regular_file() || !supported(entry.path())) continue;
                Photo photo;
                photo.path = entry.path().string();
                photo.name = entry.path().filename().string();
                photo.folder = entry.path().parent_path().string();
                photo.format = entry.path().extension().string();
                photo.bytes = static_cast<int64_t>(entry.file_size());
                const auto stamp = entry.last_write_time().time_since_epoch().count();
                photo.captured = std::to_string(stamp);
                m_store.upsert(photo);
            }
        } catch (...) {
        }
        loop.post([this, done = std::move(done)] {
            changed();
            if (done) done();
        });
    });
}
} // namespace Photos
