#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

class SwiftClickPopup : public Popup {
    protected:
    geode::TextInput* m_valueInput = nullptr;
    CCMenuItemToggler* m_toggle = nullptr;
    int m_clicks = 2;
    bool m_enabled = false;
    
    bool init() {
        if (!Popup::init(260.f, 190.f)) return false;
        
        this->setTitle("Swift Click");
        
        m_enabled = Mod::get()->getSavedValue<bool>("sc-enabled", false);
        m_clicks  = Mod::get()->getSavedValue<int>("sc-clicks", 2);
        if (m_clicks < 2) m_clicks = 2;
        
        auto contentSize = m_mainLayer->getContentSize();
        float cx = contentSize.width / 2;
        float cy = contentSize.height / 2;
        
        // enabled row
        m_toggle = CCMenuItemExt::createTogglerWithStandardSprites(
            0.7f,
            [this](CCMenuItemToggler* btn) {
                m_enabled = !m_toggle->isToggled();
                Mod::get()->setSavedValue("sc-enabled", m_enabled);
            }
        );
        m_toggle->setPosition({cx - 40.f, cy + 30.f});
        if (m_enabled) {
            m_toggle->toggle(m_enabled);
        }
        m_buttonMenu->addChild(m_toggle);
        
        auto enabledLbl = CCLabelBMFont::create("Enabled", "bigFont.fnt");
        enabledLbl->setScale(0.55f);
        enabledLbl->setAnchorPoint({0.f, 0.5f});
        enabledLbl->setPosition({cx - 19.f, cy + 30.f});
        m_mainLayer->addChild(enabledLbl);
        
        // clicks per frame row
        auto clicksLbl = CCLabelBMFont::create("Clicks / Frame", "bigFont.fnt");
        clicksLbl->setScale(0.48f);
        clicksLbl->setPosition({cx, cy - 14.f});
        m_mainLayer->addChild(clicksLbl);
        
        // value input
        m_valueInput = TextInput::create(52.f, "Num", "bigFont.fnt");
        if (m_valueInput) {
            m_valueInput->setPosition({cx, cy - 44.f});
            m_valueInput->setCommonFilter(CommonFilter::Uint);
            m_valueInput->setCallback([this](std::string const& str) {
                if (str.empty()) {
                    m_clicks = 2;
                    Mod::get()->setSavedValue("sc-clicks", m_clicks);
                    return;
                }
                int val = geode::utils::numFromString<int>(str).unwrapOr(2);
                
                if (val < 2) {
                    val = 2;
                    m_valueInput->setString("2", false);
                }
                m_clicks = val;
                Mod::get()->setSavedValue("sc-clicks", m_clicks);
            });
            m_mainLayer->addChild(m_valueInput);
        }
        
        updateValueLabel();
        
        // left arrow (decrease)
        auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto leftBtn = CCMenuItemExt::createSpriteExtra(
            leftSpr,
            [this](CCMenuItemSpriteExtra* btn) {
                if (m_clicks > 2) {
                    m_clicks--;
                    Mod::get()->setSavedValue("sc-clicks", m_clicks);
                    updateValueLabel();
                }
            }
        );
        leftBtn->setPosition({cx - 50.f, cy - 44.f});
        m_buttonMenu->addChild(leftBtn);
        
        // right arrow (increase)
        auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        rightSpr->setFlipX(true);
        auto rightBtn = CCMenuItemExt::createSpriteExtra(
            rightSpr,
            [this](CCMenuItemSpriteExtra* btn) {
                m_clicks++;
                Mod::get()->setSavedValue("sc-clicks", m_clicks);
                updateValueLabel();
            }
        );
        rightBtn->setPosition({cx + 50.f, cy - 44.f});
        m_buttonMenu->addChild(rightBtn);
        
        return true;
    }
    
    void updateValueLabel() {
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
        
        auto myButton = CCMenuItemExt::createSpriteExtra(
            mySprite,
            [this](CCMenuItemSpriteExtra* btn) {
                openPopup();
            }
        );
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
        
        auto btn = CCMenuItemExt::createSpriteExtra(
            mySprite,
            [this](CCMenuItemSpriteExtra* btn) {
                openPopup();
            }
        );
        
        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return;
        menu->addChild(btn);
        menu->updateLayout();
    }
};

// the magic hook that lets us swift click
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