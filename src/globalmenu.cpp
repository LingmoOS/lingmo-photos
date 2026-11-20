// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "globalmenu.h"
#include <base/event_loop.h>
#include <base/i18n.h>
namespace Photos {
using namespace Lingmo::base::dbus;
using Lingmo::i18n::tr;
namespace { constexpr auto path = "/org/lingmo/Photos/Menu"; constexpr auto iface = "com.canonical.dbusmenu"; constexpr auto registrar = "com.canonical.AppMenu.Registrar"; constexpr auto registrarPath = "/com/canonical/AppMenu/Registrar"; }
static std::string label(int id) { switch (id) { case 1: return tr("Photos"); case 2: return tr("Import Photos"); case 3: return tr("Quit"); case 4: return tr("Help"); case 5: return tr("About Photos"); case 6: return tr("View"); case 7: return tr("Previous"); case 8: return tr("Next"); case 9: return tr("Zoom Out"); case 10: return tr("Zoom In"); case 11: return tr("Flip"); case 12: return tr("Full Screen"); case 13: return tr("Slideshow"); case 14: return tr("Restore"); default: return {}; } }
GlobalMenu::GlobalMenu(Connection* bus, std::function<void(int32_t)> activate) : m_bus(bus), m_activate(std::move(activate)), m_object(bus, path, iface) {
    if (!bus) return;
    m_object.addProperty("Version", [] { return uint32_t(3); }); m_object.addProperty("TextDirection", [] { return std::string("ltr"); }); m_object.addProperty("Status", [] { return std::string("normal"); }); m_object.addProperty("IconThemePath", [] { return std::vector<std::string>(); });
    m_object.addMethod("GetLayout", "iias", "u(ia{sv}av)", [](MethodCall& c) { int32_t parent = 0, depth = -1; std::vector<std::string> names; c.read(parent, depth, names); using V = Variant; using M = VariantMap; std::vector<V> items; auto add = [&](int id, std::string text, bool submenu = false) { M p; p["label"] = V(std::move(text)); p["enabled"] = V(true); p["visible"] = V(true); if (submenu) p["children-display"] = V(std::string("submenu")); items.emplace_back(std::tuple<int32_t, M, std::vector<V>>(id, std::move(p), std::vector<V>{})); }; if (parent == 0) { add(1, label(1)); add(6, label(6), true); add(4, label(4), true); } else if (parent == 1) { add(2, label(2)); add(3, label(3)); } else if (parent == 6) { for (int id = 7; id <= 14; ++id) add(id, label(id)); } else if (parent == 4) add(5, label(5)); c.reply(uint32_t(1), std::tuple<int32_t, M, std::vector<V>>(parent, M{}, std::move(items))); });
    m_object.addMethod("GetGroupProperties", "aias", "a(ia{sv})", [](MethodCall& c) { std::vector<int32_t> ids; std::vector<std::string> names; c.read(ids, names); std::vector<std::tuple<int32_t, VariantMap>> result; for (const auto id : ids) if (id >= 1 && id <= 5) { VariantMap p; p["label"] = Variant(label(id)); p["enabled"] = Variant(true); p["visible"] = Variant(true); result.emplace_back(id, std::move(p)); } c.reply(result); });
    m_object.addMethod("Event", "isvu", "", [this](MethodCall& c) { int32_t id = 0; std::string event; c.read(id, event); c.reply(); if (event == "clicked" && id >= 2 && id <= 14 && id != 4 && id != 6) { auto activate = m_activate; auto alive = m_alive; base::EventLoop::current()->post([activate, id, alive] { if (*alive && activate) activate(id); }); } });
    m_object.addMethod("AboutToShow", "i", "b", [](MethodCall& c) { int32_t id = 0; c.read(id); c.reply(false); }); m_object.publish();
    m_watch = bus->watchName(registrar, [this, alive = m_alive](const std::string& owner) { if (!*alive) return; ++m_generation; m_registration.reset(); if (!owner.empty()) registerMenu(); });
}
void GlobalMenu::registerMenu() { if (!m_bus) return; const auto generation = m_generation; auto call = m_bus->newMethodCall(registrar, registrarPath, registrar, "RegisterWindow"); call.append(uint32_t(1), std::string(path)); auto alive = m_alive; m_registration = m_bus->callAsync(call, [this, alive, generation](const Reply& reply) { if (!*alive || generation != m_generation) return; const bool available = !reply.isError(); if (available != m_available) { m_available = available; availabilityChanged(available); } }); }
GlobalMenu::~GlobalMenu() { *m_alive = false; m_registration.reset(); m_watch = {}; if (m_available && m_bus) { auto call = m_bus->newMethodCall(registrar, registrarPath, registrar, "UnregisterWindow"); call.append(uint32_t(1)); m_bus->send(call); } }
} // namespace Photos
