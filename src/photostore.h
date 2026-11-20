// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Photos {

struct Photo {
    int64_t id = 0;
    std::string path;
    std::string name;
    std::string folder;
    std::string captured;
    std::string format;
    int width = 0;
    int height = 0;
    int64_t bytes = 0;
    bool favorite = false;
    bool trashed = false;
};

class PhotoStore {
public:
    explicit PhotoStore(std::string path = {});
    ~PhotoStore();
    PhotoStore(const PhotoStore&) = delete;
    PhotoStore& operator=(const PhotoStore&) = delete;

    void upsert(const Photo& photo);
    std::vector<Photo> photos(bool favorites = false, bool trash = false) const;
    std::vector<Photo> search(const std::string& query) const;
    void setFavorite(int64_t id, bool favorite);
    void setTrashed(int64_t id, bool trashed);
    size_t count(bool trash = false) const;

private:
    struct Private;
    Private* m_private;
};

} // namespace Photos
