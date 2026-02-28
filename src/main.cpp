#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

// ── Swift Click Popup ────────────────────────────────────────────────────────

class SwiftClickPopup : public Popup {
protected:
    CCLabelBMFont* m_valueLabel = nullptr;
    CCMenuItemToggler* m_toggle = nullptr;
    int m_clicks = 2;
    bool m_enabled = false;

    bool init() {
        if (!Popup::init(260.f, 190.f))
            return false;

        m_enabled = Mod::get()->getSavedValue<bool>("sc-enabled", false);
        m_clicks  = Mod::get()->getSavedValue<int>("sc-clicks", 2);

        this->setTitle("Swift Click");

        auto contentSize = m_mainLayer->getContentSize();
        float cx = contentSize.width / 2;
        float cy = contentSize.height / 2;

        // ── Enabled row ──────────────────────────────────────────────────
        auto enabledLbl = CCLabelBMFont::create("Enabled", "bigFont.fnt");
        enabledLbl->setScale(0.55f);
        enabledLbl->setAnchorPoint({1.f, 0.5f});
        enabledLbl->setPosition({cx - 10.f, cy + 24.f});
        m_mainLayer->addChild(enabledLbl);

        m_toggle = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(SwiftClickPopup::onToggle), 0.7f
        );
        m_toggle->setPosition({cx + 18.f, cy + 24.f});
        // Only call toggle() if enabled, toggler starts OFF by default
        if (m_enabled) {
            m_toggle->toggle(m_enabled);
        }
        m_buttonMenu->addChild(m_toggle);

        // ── Clicks per frame row ─────────────────────────────────────────
        auto clicksLbl = CCLabelBMFont::create("Clicks / Frame", "bigFont.fnt");
        clicksLbl->setScale(0.48f);
        clicksLbl->setPosition({cx, cy - 14.f});
        m_mainLayer->addChild(clicksLbl);

        // Value box
        auto box = CCScale9Sprite::create("square02_small.png");
        if (!box) box = CCScale9Sprite::create("GJ_square07.png");
        if (box) {
            box->setContentSize({52.f, 28.f});
            box->setOpacity(100);
            box->setPosition({cx, cy - 44.f});
            m_mainLayer->addChild(box);
        }

        // Value label
        m_valueLabel = CCLabelBMFont::create("2", "bigFont.fnt");
        m_valueLabel->setScale(0.6f);
        m_valueLabel->setPosition({cx, cy - 44.f});
        m_mainLayer->addChild(m_valueLabel);
        updateValueLabel();

        // Left arrow ◀ (decrease) — default sprite faces left, no flip needed
        auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto leftBtn = CCMenuItemSpriteExtra::create(
            leftSpr, this, menu_selector(SwiftClickPopup::onDecrease)
        );
        leftBtn->setPosition({cx - 50.f, cy - 44.f});
        m_buttonMenu->addChild(leftBtn);

        // Right arrow ▶ (increase) — flip to face right
        auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        rightSpr->setFlipX(true);
        auto rightBtn = CCMenuItemSpriteExtra::create(
            rightSpr, this, menu_selector(SwiftClickPopup::onIncrease)
        );
        rightBtn->setPosition({cx + 50.f, cy - 44.f});
        m_buttonMenu->addChild(rightBtn);

        return true;
    }

    void updateValueLabel() {
        m_valueLabel->setString(std::to_string(m_clicks).c_str());
    }

    void onToggle(CCObject*) {
        m_enabled = !m_toggle->isToggled();
        Mod::get()->setSavedValue("sc-enabled", m_enabled);
    }

    void onDecrease(CCObject*) {
        if (m_clicks > 2) {
            m_clicks--;
            Mod::get()->setSavedValue("sc-clicks", m_clicks);
            updateValueLabel();
        }
    }

    void onIncrease(CCObject*) {
        if (m_clicks < 20) {
            m_clicks++;
            Mod::get()->setSavedValue("sc-clicks", m_clicks);
            updateValueLabel();
        }
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

static void openPopup() {
    SwiftClickPopup::create()->show();
}

// ── Main Menu Button ─────────────────────────────────────────────────────────

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto myButton = CCMenuItemSpriteExtra::create(
            CCSprite::create("swiftclicks.png"_spr),
            this,
            menu_selector(MyMenuLayer::onSwiftClick)
        );

        auto menu = this->getChildByID("bottom-menu");
        menu->addChild(myButton);
        myButton->setID("swift-click-btn"_spr);
        menu->updateLayout();

        return true;
    }

    void onSwiftClick(CCObject*) {
        openPopup();
    }
};

// ── Pause Menu Button ─────────────────────────────────────────────────────────

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto btn = CCMenuItemSpriteExtra::create(
            CCSprite::create("swiftclicks.png"_spr),
            this,
            menu_selector(MyPauseLayer::onSwiftClick)
        );
        btn->setScale(0.85f);

        auto menu = CCMenu::create();
        menu->setPosition({winSize.width - 35.f, 35.f});
        menu->addChild(btn);
        this->addChild(menu, 10);
    }

    void onSwiftClick(CCObject*) {
        openPopup();
    }
};

// ── Click Simulation Hook ─────────────────────────────────────────────────────

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool push, int button, bool isPrimary) {
        GJBaseGameLayer::handleButton(push, button, isPrimary);

        if (!push) return;
        if (!Mod::get()->getSavedValue<bool>("sc-enabled", false)) return;

        int clicks = Mod::get()->getSavedValue<int>("sc-clicks", 2);
        if (clicks < 2) clicks = 2;

        for (int i = 1; i < clicks; i++) {
            GJBaseGameLayer::handleButton(false, button, isPrimary);
            GJBaseGameLayer::handleButton(true, button, isPrimary);
        }
    }
};