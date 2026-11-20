#include "photostore.h"
#include <gtest/gtest.h>
#include <filesystem>
TEST(PhotoStoreTest, PersistsAndFiltersPhotos) { const auto path = std::filesystem::temp_directory_path() / "lingmo-photos-test.db"; Photos::PhotoStore store(path.string()); Photos::Photo photo; photo.path = "/tmp/photo.jpg"; photo.name = "photo.jpg"; photo.folder = "/tmp"; photo.captured = "2026-11-21"; photo.format = ".jpg"; store.upsert(photo); EXPECT_EQ(store.count(), 1u); EXPECT_EQ(store.search("photo").size(), 1u); store.setFavorite(1, true); EXPECT_EQ(store.photos(true).size(), 1u); }
