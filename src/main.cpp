#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

using namespace geode::prelude;

static bool g_enabled       = false;
static int  g_clicks        = 2;
static bool g_processing    = false;
static bool g_usedThisLevel = false;

struct PercentRule {
    int percentage;
    int clicks;
};

static std::vector<PercentRule> g_percentRules;

static void syncCache() {
    g_enabled = Mod::get()->getSavedValue<bool>("sc-enabled", false);
    g_clicks  = std::max(2, Mod::get()->getSavedValue<int>("sc-clicks", 2));
}

static void savePercentRules() {
    std::vector<matjson::Value> arr;
    for (auto& r : g_percentRules) {
        matjson::Value obj = matjson::makeObject({
            { "percentage", r.percentage },
            { "clicks",     r.clicks     }
        });
        arr.push_back(std::move(obj));
    }
    Mod::get()->setSavedValue("sc-percent-rules", matjson::Value(arr));
}

static void loadPercentRules() {
    g_percentRules.clear();
    auto saved = Mod::get()->getSavedValue<matjson::Value>(
        "sc-percent-rules",
        matjson::Value(std::vector<matjson::Value>{})
    );
    if (!saved.isArray()) return;
    auto arrRes = saved.asArray();
    if (!arrRes) return;
    for (auto& v : arrRes.unwrap()) {
        if (!v.isObject()) continue;
        PercentRule r;
        r.percentage = v["percentage"].isNumber() ? v["percentage"].asInt().unwrapOr(0) : 0;
        r.clicks     = v["clicks"].isNumber()     ? v["clicks"].asInt().unwrapOr(2)     : 2;
        g_percentRules.push_back(r);
    }
}

$execute {
    syncCache();
    loadPercentRules();
}


class AdvancedPopup : public Popup {
protected:
    static constexpr float PW         = 340.f;
    static constexpr float PH         = 280.f;
    static constexpr float SCROLL_X   = 10.f;
    static constexpr float SCROLL_Y   = 36.f;
    static constexpr float SCROLL_W   = 320.f;
    static constexpr float SCROLL_H   = 180.f;
    static constexpr float ROW_H      = 52.f;
    static constexpr float ROW_GAP    = 6.f;
    static constexpr float ROW_FULL   = ROW_H + ROW_GAP;
    static constexpr float DEL_W      = 36.f;
    static constexpr float PAD        = 6.f;
    static constexpr float INPUT_INSET = 10.f;

    CCNode*      m_content     = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;
    CCNode*      m_clipNode    = nullptr;

    bool init() {
        if (!Popup::init(PW, PH)) return false;
        this->setTitle("Advanced");
        addColumnHeaders();
        buildScroll(0.f);
        addPlusButton();
        return true;
    }

    void addColumnHeaders() {
        float usableW = SCROLL_W - DEL_W - PAD * 2.f;
        float colW    = usableW / 2.f;
        float col1Cx  = SCROLL_X + PAD + colW * 0.5f;
        float col2Cx  = SCROLL_X + PAD + colW + colW * 0.5f;
        float headerY = SCROLL_Y + SCROLL_H + 8.f;

        auto pctHdr = CCLabelBMFont::create("Percentage %", "goldFont.fnt");
        pctHdr->setScale(0.42f);
        pctHdr->setAnchorPoint({0.5f, 0.f});
        pctHdr->setPosition({col1Cx, headerY});
        m_mainLayer->addChild(pctHdr);

        auto clkHdr = CCLabelBMFont::create("Clicks/Frame", "goldFont.fnt");
        clkHdr->setScale(0.42f);
        clkHdr->setAnchorPoint({0.5f, 0.f});
        clkHdr->setPosition({col2Cx, headerY});
        m_mainLayer->addChild(clkHdr);
    }

    void addPlusButton() {
        auto addSpr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
        addSpr->setScale(0.7f);
        auto addBtn = CCMenuItemExt::createSpriteExtra(addSpr, [this](CCMenuItemSpriteExtra*) {
            float scrollOffset = m_scrollLayer ? m_scrollLayer->m_contentLayer->getPositionY() : 0.f;
            g_percentRules.push_back({0, 2});
            savePercentRules();
            fullRebuild(true, scrollOffset);
        });
        addBtn->setPosition({PW - 20.f, PH - 20.f});
        m_buttonMenu->addChild(addBtn);
    }

    float contentHeight() const {
        int n = (int)g_percentRules.size();
        if (n == 0) return SCROLL_H;
        return std::max(SCROLL_H, (float)(n * ROW_FULL) + ROW_GAP);
    }

    void buildScroll(float restoreScrollY) {
        if (m_clipNode) {
            m_clipNode->removeFromParent();
            m_clipNode    = nullptr;
            m_scrollLayer = nullptr;
            m_content     = nullptr;
        }

        auto clipNode = CCClippingNode::create();
        clipNode->setContentSize({SCROLL_W, SCROLL_H});
        clipNode->setPosition({SCROLL_X, SCROLL_Y});
        clipNode->setAnchorPoint({0.f, 0.f});

        auto stencil = CCScale9Sprite::create("square02_small.png");
        stencil->setContentSize({SCROLL_W, SCROLL_H});
        stencil->setPosition({SCROLL_W / 2.f, SCROLL_H / 2.f});
        clipNode->setStencil(stencil);
        clipNode->setAlphaThreshold(0.05f);

        m_mainLayer->addChild(clipNode, 2);
        m_clipNode = clipNode;

        m_scrollLayer = ScrollLayer::create({SCROLL_W, SCROLL_H});
        m_scrollLayer->setPosition({0.f, 0.f});
        m_scrollLayer->setAnchorPoint({0.f, 0.f});
        clipNode->addChild(m_scrollLayer);

        float cH = contentHeight();
        m_content = CCNode::create();
        m_content->setContentSize({SCROLL_W, cH});
        m_content->setPosition({0.f, 0.f});
        m_content->setAnchorPoint({0.f, 0.f});
        m_scrollLayer->m_contentLayer->addChild(m_content);
        m_scrollLayer->m_contentLayer->setContentSize({SCROLL_W, cH});

        if (g_percentRules.empty()) {
            auto hint = CCLabelBMFont::create("Click + to add a rule.", "chatFont.fnt");
            hint->setScale(0.5f);
            hint->setColor({200, 200, 200});
            hint->setPosition({SCROLL_W / 2.f, cH / 2.f});
            m_content->addChild(hint);
            m_scrollLayer->scrollToTop();
            return;
        }

        float y = cH - ROW_GAP - ROW_H / 2.f;
        for (int i = 0; i < (int)g_percentRules.size(); i++) {
            addRow(i, y);
            y -= ROW_FULL;
        }

        float maxOffset = std::max(0.f, cH - SCROLL_H);
        float clampedY  = std::clamp(restoreScrollY, -maxOffset, 0.f);
        m_scrollLayer->m_contentLayer->setPositionY(clampedY);
    }

    void addRow(int idx, float y) {
        auto& rule    = g_percentRules[idx];
        float usableW = SCROLL_W - DEL_W - PAD * 2.f;
        float colW    = usableW / 2.f;
        float col1Cx  = PAD + colW * 0.5f;
        float col2Cx  = PAD + colW + colW * 0.5f;
        float boxH    = ROW_H - 8.f;

        auto bg1 = CCScale9Sprite::create("square02_small.png");
        bg1->setContentSize({colW - 4.f, boxH});
        bg1->setColor({0, 0, 0});
        bg1->setOpacity(60);
        bg1->setAnchorPoint({0.5f, 0.5f});
        bg1->setPosition({col1Cx, y});
        m_content->addChild(bg1);

        auto bg2 = CCScale9Sprite::create("square02_small.png");
        bg2->setContentSize({colW - 4.f, boxH});
        bg2->setColor({0, 0, 0});
        bg2->setOpacity(60);
        bg2->setAnchorPoint({0.5f, 0.5f});
        bg2->setPosition({col2Cx, y});
        m_content->addChild(bg2);

        float inputW = colW - 4.f - INPUT_INSET * 2.f;

        auto pctInput = TextInput::create(inputW, "%", "bigFont.fnt");
        pctInput->setPosition({col1Cx, y});
        pctInput->setCommonFilter(CommonFilter::Uint);
        pctInput->setString(geode::utils::numToString(rule.percentage));
        pctInput->setCallback([this, idx](std::string const& s) {
            int v = std::clamp(geode::utils::numFromString<int>(s).unwrapOr(0), 0, 100);
            g_percentRules[idx].percentage = v;
            savePercentRules();
        });
        m_content->addChild(pctInput);

        auto clkInput = TextInput::create(inputW, "C/F", "bigFont.fnt");
        clkInput->setPosition({col2Cx, y});
        clkInput->setCommonFilter(CommonFilter::Uint);
        clkInput->setString(geode::utils::numToString(rule.clicks));
        clkInput->setCallback([this, idx](std::string const& s) {
            int v = geode::utils::numFromString<int>(s).unwrapOr(2);
            if (v < 2) v = 2;
            g_percentRules[idx].clicks = v;
            savePercentRules();
        });
        m_content->addChild(clkInput);

        auto delMenu = CCMenu::create();
        delMenu->setPosition({0.f, 0.f});
        delMenu->setAnchorPoint({0.f, 0.f});
        delMenu->ignoreAnchorPointForPosition(true);
        m_content->addChild(delMenu, 5);

        auto delSpr = CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png");
        delSpr->setScale(0.65f);
        auto delBtn = CCMenuItemExt::createSpriteExtra(delSpr, [this, idx](CCMenuItemSpriteExtra*) {
            float scrollY = m_scrollLayer ? m_scrollLayer->m_contentLayer->getPositionY() : 0.f;
            g_percentRules.erase(g_percentRules.begin() + idx);
            savePercentRules();
            fullRebuild(false, scrollY);
        });
        delBtn->setPosition({SCROLL_W - DEL_W / 2.f, y});
        delMenu->addChild(delBtn);
    }

    void fullRebuild(bool scrollToBottom, float restoreY) {
        m_buttonMenu->removeAllChildrenWithCleanup(true);

        auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        closeSpr->setScale(0.8f);
        auto closeBtn = CCMenuItemExt::createSpriteExtra(closeSpr, [this](CCMenuItemSpriteExtra*) {
            this->onClose(nullptr);
        });
        closeBtn->setPosition({3.f, PH - 3.f});
        m_buttonMenu->addChild(closeBtn);

        addPlusButton();

        float cH        = (float)g_percentRules.size() * ROW_FULL + ROW_GAP;
        float maxOffset = std::max(0.f, cH - SCROLL_H);
        float targetY   = scrollToBottom ? -maxOffset : restoreY;

        buildScroll(targetY);
    }

public:
    static AdvancedPopup* create() {
        auto ret = new AdvancedPopup();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }
};


class SwiftClickPopup : public Popup {
protected:
    geode::TextInput*  m_valueInput = nullptr;
    CCMenuItemToggler* m_toggle     = nullptr;
    int                m_clicks     = 2;
    bool               m_enabled    = false;

    bool init() {
        if (!Popup::init(260.f, 210.f)) return false;
        this->setTitle("Swift Click");

        m_enabled = Mod::get()->getSavedValue<bool>("sc-enabled", false);
        m_clicks  = std::max(2, Mod::get()->getSavedValue<int>("sc-clicks", 2));

        auto contentSize = m_mainLayer->getContentSize();
        float cx = contentSize.width  / 2.f;
        float cy = contentSize.height / 2.f;

        m_toggle = CCMenuItemExt::createTogglerWithStandardSprites(
            0.7f,
            [this](CCMenuItemToggler*) {
                m_enabled = !m_toggle->isToggled();
                Mod::get()->setSavedValue("sc-enabled", m_enabled);
                syncCache();
            }
        );
        m_toggle->setPosition({cx - 40.f, cy + 40.f});
        if (m_enabled) m_toggle->toggle(m_enabled);
        m_buttonMenu->addChild(m_toggle);

        auto enabledLbl = CCLabelBMFont::create("Enabled", "bigFont.fnt");
        enabledLbl->setScale(0.55f);
        enabledLbl->setAnchorPoint({0.f, 0.5f});
        enabledLbl->setPosition({cx - 19.f, cy + 40.f});
        m_mainLayer->addChild(enabledLbl);

        auto clicksLbl = CCLabelBMFont::create("Clicks / Frame", "bigFont.fnt");
        clicksLbl->setScale(0.48f);
        clicksLbl->setPosition({cx, cy - 4.f});
        m_mainLayer->addChild(clicksLbl);

        m_valueInput = TextInput::create(52.f, "Num", "bigFont.fnt");
        if (m_valueInput) {
            m_valueInput->setPosition({cx, cy - 34.f});
            m_valueInput->setCommonFilter(CommonFilter::Uint);
            m_valueInput->setCallback([this](std::string const& str) {
                int val = geode::utils::numFromString<int>(str).unwrapOr(2);
                if (val < 2) {
                    val = 2;
                    m_valueInput->setString("2", false);
                }
                m_clicks = val;
                Mod::get()->setSavedValue("sc-clicks", m_clicks);
                syncCache();
            });
            m_mainLayer->addChild(m_valueInput);
        }
        updateValueLabel();

        auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto leftBtn = CCMenuItemExt::createSpriteExtra(leftSpr, [this](CCMenuItemSpriteExtra*) {
            if (m_clicks > 2) {
                m_clicks--;
                Mod::get()->setSavedValue("sc-clicks", m_clicks);
                syncCache();
                updateValueLabel();
            }
        });
        leftBtn->setPosition({cx - 50.f, cy - 34.f});
        m_buttonMenu->addChild(leftBtn);

        auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        rightSpr->setFlipX(true);
        auto rightBtn = CCMenuItemExt::createSpriteExtra(rightSpr, [this](CCMenuItemSpriteExtra*) {
            m_clicks++;
            Mod::get()->setSavedValue("sc-clicks", m_clicks);
            syncCache();
            updateValueLabel();
        });
        rightBtn->setPosition({cx + 50.f, cy - 34.f});
        m_buttonMenu->addChild(rightBtn);

        auto gearSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        gearSpr->setScale(0.65f);
        auto advBtn = CCMenuItemExt::createSpriteExtra(gearSpr, [this](CCMenuItemSpriteExtra*) {
            AdvancedPopup::create()->show();
        });
        advBtn->setPosition({contentSize.width - 20.f, 22.f});
        m_buttonMenu->addChild(advBtn);

        return true;
    }

    void updateValueLabel() {
        if (m_valueInput)
            m_valueInput->setString(geode::utils::numToString(m_clicks));
    }

public:
    static SwiftClickPopup* create() {
        auto ret = new SwiftClickPopup();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }
};

static void openPopup() {
    SwiftClickPopup::create()->show();
}


class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto mySprite = CircleButtonSprite::create(
            CCSprite::create("swiftclicks.png"_spr),
            CircleBaseColor::Green,
            CircleBaseSize::MediumAlt
        );
        mySprite->setTopOffset(ccp(0, 2));

        auto myButton = CCMenuItemExt::createSpriteExtra(mySprite, [this](CCMenuItemSpriteExtra*) {
            openPopup();
        });

        auto menu = this->getChildByID("bottom-menu");
        menu->addChild(myButton);
        myButton->setID("swift-click-btn"_spr);
        menu->updateLayout();

        return true;
    }
};

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

        auto btn = CCMenuItemExt::createSpriteExtra(mySprite, [this](CCMenuItemSpriteExtra*) {
            openPopup();
        });

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return;
        menu->addChild(btn);
        menu->updateLayout();
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        g_usedThisLevel = false;
    }
};

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

        int effectiveClicks = g_clicks;

        if (!g_percentRules.empty() && m_player1 && m_levelLength > 0.f) {
            float pct = (m_player1->getPositionX() / m_levelLength) * 100.f;
            int bestPct = -1;
            for (auto& r : g_percentRules) {
                if ((float)r.percentage <= pct && r.percentage > bestPct) {
                    bestPct         = r.percentage;
                    effectiveClicks = r.clicks;
                }
            }
        }

        if (effectiveClicks < 2) effectiveClicks = 2;

        g_processing    = true;
        g_usedThisLevel = true;

#ifdef GEODE_IS_MOBILE
        auto player = isPrimary ? m_player1 : m_player2;
        if (player) {
            auto btn = static_cast<PlayerButton>(button);
            for (int i = 1; i < effectiveClicks; i++) {
                player->releaseButton(btn);
                player->pushButton(btn);
            }
        }
#else
        for (int i = 1; i < effectiveClicks; i++) {
            GJBaseGameLayer::handleButton(false, button, isPrimary);
            GJBaseGameLayer::handleButton(true, button, isPrimary);
        }
#endif
        g_processing = false;
    }
};