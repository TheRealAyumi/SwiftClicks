#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

using namespace geode::prelude;

// ─── Cached state ─────────────────────────────────────────────────────────────

static bool g_enabled    = false;
static int  g_clicks     = 2;
static bool g_processing = false;
static bool g_usedThisLevel = false;

static void syncCache() {
    g_enabled = Mod::get()->getSavedValue<bool>("sc-enabled", false);
    g_clicks  = std::max(2, Mod::get()->getSavedValue<int>("sc-clicks", 2));
}

$execute {
    syncCache();
}

// ─── Swift Click Popup ────────────────────────────────────────────────────────

class SwiftClickPopup : public Popup {
protected:
    geode::TextInput*    m_valueInput = nullptr;
    CCMenuItemToggler*   m_toggle     = nullptr;
    int                  m_clicks     = 2;
    bool                 m_enabled    = false;

    bool init() {
        if (!Popup::init(260.f, 190.f)) return false;

        this->setTitle("Swift Click");

        m_enabled = Mod::get()->getSavedValue<bool>("sc-enabled", false);
        m_clicks  = std::max(2, Mod::get()->getSavedValue<int>("sc-clicks", 2));

        auto contentSize = m_mainLayer->getContentSize();
        float cx = contentSize.width  / 2;
        float cy = contentSize.height / 2;

        // ── Enabled toggle ──────────────────────────────────────────────────

        m_toggle = CCMenuItemExt::createTogglerWithStandardSprites(
            0.7f,
            [this](CCMenuItemToggler* btn) {
                m_enabled = !m_toggle->isToggled();
                Mod::get()->setSavedValue("sc-enabled", m_enabled);
                syncCache();
            }
        );
        m_toggle->setPosition({cx - 40.f, cy + 30.f});
        if (m_enabled) m_toggle->toggle(m_enabled);
        m_buttonMenu->addChild(m_toggle);

        auto enabledLbl = CCLabelBMFont::create("Enabled", "bigFont.fnt");
        enabledLbl->setScale(0.55f);
        enabledLbl->setAnchorPoint({0.f, 0.5f});
        enabledLbl->setPosition({cx - 19.f, cy + 30.f});
        m_mainLayer->addChild(enabledLbl);

        // ── Clicks / Frame label ────────────────────────────────────────────

        auto clicksLbl = CCLabelBMFont::create("Clicks / Frame", "bigFont.fnt");
        clicksLbl->setScale(0.48f);
        clicksLbl->setPosition({cx, cy - 14.f});
        m_mainLayer->addChild(clicksLbl);

        // ── Value input ─────────────────────────────────────────────────────

        m_valueInput = TextInput::create(52.f, "Num", "bigFont.fnt");
        if (m_valueInput) {
            m_valueInput->setPosition({cx, cy - 44.f});
            m_valueInput->setCommonFilter(CommonFilter::Uint);
            m_valueInput->setCallback([this](std::string const& str) {
                if (str.empty()) {
                    m_clicks = 2;
                } else {
                    int val = geode::utils::numFromString<int>(str).unwrapOr(2);
                    if (val < 2) {
                        val = 2;
                        m_valueInput->setString("2", false);
                    }
                    m_clicks = val;
                }
                Mod::get()->setSavedValue("sc-clicks", m_clicks);
                syncCache();
            });
            m_mainLayer->addChild(m_valueInput);
        }

        updateValueLabel();

        // ── Left arrow (decrease) ───────────────────────────────────────────

        auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto leftBtn = CCMenuItemExt::createSpriteExtra(
            leftSpr,
            [this](CCMenuItemSpriteExtra*) {
                if (m_clicks > 2) {
                    m_clicks--;
                    Mod::get()->setSavedValue("sc-clicks", m_clicks);
                    syncCache();
                    updateValueLabel();
                }
            }
        );
        leftBtn->setPosition({cx - 50.f, cy - 44.f});
        m_buttonMenu->addChild(leftBtn);

        // ── Right arrow (increase) ──────────────────────────────────────────

        auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        rightSpr->setFlipX(true);
        auto rightBtn = CCMenuItemExt::createSpriteExtra(
            rightSpr,
            [this](CCMenuItemSpriteExtra*) {
                m_clicks++;
                Mod::get()->setSavedValue("sc-clicks", m_clicks);
                syncCache();
                updateValueLabel();
            }
        );
        rightBtn->setPosition({cx + 50.f, cy - 44.f});
        m_buttonMenu->addChild(rightBtn);

        return true;
    }

    void updateValueLabel() {
        if (m_valueInput)
            m_valueInput->setString(geode::utils::numToString(m_clicks));
    }

public:
    static SwiftClickPopup* create() {
        auto ret = new SwiftClickPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

// ─── Open popup helper ────────────────────────────────────────────────────────

static void openPopup() {
    SwiftClickPopup::create()->show();
}

// ─── MenuLayer Hook ───────────────────────────────────────────────────────────

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto mySprite = CircleButtonSprite::create(
            CCSprite::create("swiftclicks.png"_spr),
            CircleBaseColor::Green,
            CircleBaseSize::MediumAlt
        );
        mySprite->setTopOffset(ccp(0, 2));

        auto myButton = CCMenuItemExt::createSpriteExtra(
            mySprite,
            [this](CCMenuItemSpriteExtra*) { openPopup(); }
        );

        auto menu = this->getChildByID("bottom-menu");
        menu->addChild(myButton);
        myButton->setID("swift-click-btn"_spr);
        menu->updateLayout();

        return true;
    }
};

// ─── PauseLayer Hook ──────────────────────────────────────────────────────────

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto mySprite = CircleButtonSprite::create(
            CCSprite::create("swiftclicks.png"_spr),
            CircleBaseColor::Green,
            CircleBaseSize::MediumAlt
        );
        mySprite->setScale(0.6f);
        mySprite->setTopOffset(ccp(0, 2));

        auto btn = CCMenuItemExt::createSpriteExtra(
            mySprite,
            [this](CCMenuItemSpriteExtra*) { openPopup(); }
        );

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return;
        menu->addChild(btn);
        menu->updateLayout();
    }
};

// ─── PlayLayer Hook — reset flag on each attempt ─────────────────────────────

class $modify(MyPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        g_usedThisLevel = false;
    }
};

// ─── EndLevelLayer Hook — show indicator if swift click was used ──────────────

class $modify(MyEndLevelLayer, EndLevelLayer) {
    void customSetup() {
        EndLevelLayer::customSetup();

        if (!g_usedThisLevel) return;

        auto label = CCLabelBMFont::create("Swift Clicks Mod Was Used", "bigFont.fnt");
        label->setScale(0.3f);
        label->setColor({255, 100, 100});
        label->setOpacity(200);

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        label->setAnchorPoint({1.f, 0.f});
        label->setPosition({winSize.width - 10.f, 10.f});
        label->setZOrder(9999);

        this->addChild(label);
    }
};


class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool push, int button, bool isPrimary) {
        GJBaseGameLayer::handleButton(push, button, isPrimary);

        if (!push || !g_enabled || g_processing) return;

        g_processing = true;
        g_usedThisLevel = true;
        for (int i = 1; i < g_clicks; i++) {
            GJBaseGameLayer::handleButton(false, button, isPrimary);
            GJBaseGameLayer::handleButton(true, button, isPrimary);
        }
        g_processing = false;
    }
};