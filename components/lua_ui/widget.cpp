#include "widget.hpp"

#include <SDL_events.h>
#include <components/sdlutil/sdlmappings.hpp>

#include "text.hpp"
#include "textedit.hpp"
#include "window.hpp"

namespace LuaUi
{
    WidgetExtension::WidgetExtension()
        : mPropagateEvents(true)
        , mLua(nullptr)
        , mWidget(nullptr)
        , mSlot(this)
        , mLayout(sol::nil)
        , mProperties(sol::nil)
        , mTemplateProperties(sol::nil)
        , mExternal(sol::nil)
        , mParent(nullptr)
        , mTemplateChild(false)
    {}

    void WidgetExtension::initialize(lua_State* lua, MyGUI::Widget* self)
    {
        mLua = lua;
        mWidget = self;
        initialize();
        updateTemplate();
    }

    void WidgetExtension::initialize()
    {
        // \todo might be more efficient to only register these if there are Lua callbacks
        mWidget->eventKeyButtonPressed += MyGUI::newDelegate(this, &WidgetExtension::keyPress);
        mWidget->eventKeyButtonReleased += MyGUI::newDelegate(this, &WidgetExtension::keyRelease);
        mWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &WidgetExtension::mouseClick);
        mWidget->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &WidgetExtension::mouseDoubleClick);
        mWidget->eventMouseButtonPressed += MyGUI::newDelegate(this, &WidgetExtension::mousePress);
        mWidget->eventMouseButtonReleased += MyGUI::newDelegate(this, &WidgetExtension::mouseRelease);
        mWidget->eventMouseMove += MyGUI::newDelegate(this, &WidgetExtension::mouseMove);
        mWidget->eventMouseDrag += MyGUI::newDelegate(this, &WidgetExtension::mouseDrag);

        mWidget->eventMouseSetFocus += MyGUI::newDelegate(this, &WidgetExtension::focusGain);
        mWidget->eventMouseLostFocus += MyGUI::newDelegate(this, &WidgetExtension::focusLoss);
        mWidget->eventKeySetFocus += MyGUI::newDelegate(this, &WidgetExtension::focusGain);
        mWidget->eventKeyLostFocus += MyGUI::newDelegate(this, &WidgetExtension::focusLoss);
    }

    void WidgetExtension::deinitialize()
    {
        clearCallbacks();
        mWidget->eventKeyButtonPressed.clear();
        mWidget->eventKeyButtonReleased.clear();
        mWidget->eventMouseButtonClick.clear();
        mWidget->eventMouseButtonDoubleClick.clear();
        mWidget->eventMouseButtonPressed.clear();
        mWidget->eventMouseButtonReleased.clear();
        mWidget->eventMouseMove.clear();
        mWidget->eventMouseDrag.m_event.clear();

        mWidget->eventMouseSetFocus.clear();
        mWidget->eventMouseLostFocus.clear();
        mWidget->eventKeySetFocus.clear();
        mWidget->eventKeyLostFocus.clear();

        mOnCoordChange.reset();

        for (WidgetExtension* w : mChildren)
            w->deinitialize();
        for (WidgetExtension* w : mTemplateChildren)
            w->deinitialize();
    }

    void WidgetExtension::reset()
    {
        // detach all children from the slot widget, in case it gets destroyed
        for (auto& w: mChildren)
            w->widget()->detachFromWidget();
    }

    void WidgetExtension::attach(WidgetExtension* ext)
    {
        ext->mParent = this;
        ext->mTemplateChild = false;
        ext->widget()->attachToWidget(mSlot->widget());
        ext->updateCoord();
    }

    void WidgetExtension::attachTemplate(WidgetExtension* ext)
    {
        ext->mParent = this;
        ext->mTemplateChild = true;
        ext->widget()->attachToWidget(widget());
        ext->updateCoord();
    }

    WidgetExtension* WidgetExtension::findDeep(std::string_view flagName)
    {
        for (WidgetExtension* w : mChildren)
        {
            WidgetExtension* result = w->findDeep(flagName);
            if (result != nullptr)
                return result;
        }
        if (externalValue(flagName, false))
            return this;
        return nullptr;
    }

    void WidgetExtension::findAll(std::string_view flagName, std::vector<WidgetExtension*>& result)
    {
        if (externalValue(flagName, false))
            result.push_back(this);
        for (WidgetExtension* w : mChildren)
            w->findAll(flagName, result);
    }

    WidgetExtension* WidgetExtension::findDeepInTemplates(std::string_view flagName)
    {
        for (WidgetExtension* w : mTemplateChildren)
        {
            WidgetExtension* result = w->findDeep(flagName);
            if (result != nullptr)
                return result;
        }
        return nullptr;
    }

    std::vector<WidgetExtension*> WidgetExtension::findAllInTemplates(std::string_view flagName)
    {
        std::vector<WidgetExtension*> result;
        for (WidgetExtension* w : mTemplateChildren)
            w->findAll(flagName, result);
        return result;
    }

    sol::table WidgetExtension::makeTable() const
    {
        return sol::table(lua(), sol::create);
    }

    sol::object WidgetExtension::keyEvent(MyGUI::KeyCode code) const
    {
        SDL_Keysym keySym;
        keySym.sym = SDLUtil::myGuiKeyToSdl(code);
        keySym.scancode = SDL_GetScancodeFromKey(keySym.sym);
        keySym.mod = SDL_GetModState();
        return sol::make_object(lua(), keySym);
    }

    sol::object WidgetExtension::mouseEvent(int left, int top, MyGUI::MouseButton button = MyGUI::MouseButton::None) const
    {
        osg::Vec2f position(left, top);
        MyGUI::IntPoint absolutePosition = mWidget->getAbsolutePosition();
        osg::Vec2f offset = position - osg::Vec2f(absolutePosition.left, absolutePosition.top);
        sol::table table = makeTable();
        table["position"] = position;
        table["offset"] = offset;
        table["button"] = SDLUtil::myGuiMouseButtonToSdl(button);
        return table;
    }

    void WidgetExtension::setChildren(const std::vector<WidgetExtension*>& children)
    {
        mChildren.resize(children.size());
        for (size_t i = 0; i < children.size(); ++i)
        {
            mChildren[i] = children[i];
            attach(mChildren[i]);
        }
        updateChildren();
    }

    void WidgetExtension::setTemplateChildren(const std::vector<WidgetExtension*>& children)
    {
        mTemplateChildren.resize(children.size());
        for (size_t i = 0; i < children.size(); ++i)
        {
            mTemplateChildren[i] = children[i];
            attachTemplate(mTemplateChildren[i]);
        }
        updateTemplate();
    }

    void WidgetExtension::updateTemplate()
    {
        WidgetExtension* oldSlot = mSlot;
        WidgetExtension* slot = findDeepInTemplates("slot");
        if (slot == nullptr)
            mSlot = this;
        else
            mSlot = slot->mSlot;
        if (mSlot != oldSlot)
        {
            MyGUI::IntSize slotSize = mSlot->widget()->getSize();
            MyGUI::IntPoint slotPosition = mSlot->widget()->getAbsolutePosition() - widget()->getAbsolutePosition();
            MyGUI::IntCoord slotCoord(slotPosition, slotSize);
            if (mWidget->getSubWidgetMain())
                mWidget->getSubWidgetMain()->setCoord(slotCoord);
            if (mWidget->getSubWidgetText())
                mWidget->getSubWidgetText()->setCoord(slotCoord);
        }
    }

    void WidgetExtension::setCallback(const std::string& name, const LuaUtil::Callback& callback)
    {
        mCallbacks[name] = callback;
    }

    void WidgetExtension::clearCallbacks()
    {
        mCallbacks.clear();
    }

    MyGUI::IntCoord WidgetExtension::forcedCoord()
    {
        return mForcedCoord;
    }

    void WidgetExtension::setForcedCoord(const MyGUI::IntCoord& offset)
    {
        mForcedCoord = offset;
    }

    void WidgetExtension::setForcedSize(const MyGUI::IntSize& size)
    {
        mForcedCoord = size;
    }

    void WidgetExtension::updateCoord()
    {
        MyGUI::IntCoord oldCoord = mWidget->getCoord();
        MyGUI::IntCoord newCoord = calculateCoord();

        if (oldCoord != newCoord)
            mWidget->setCoord(newCoord);
        if (oldCoord.size() != newCoord.size())
            updateChildrenCoord();
        if (oldCoord != newCoord && mOnCoordChange.has_value())
            mOnCoordChange.value()(this, newCoord);
    }

    void WidgetExtension::setProperties(sol::object props)
    {
        mProperties = props;
        updateProperties();
        updateCoord();
    }

    void WidgetExtension::updateProperties()
    {
        mPropagateEvents = propertyValue("propagateEvents", true);
        mAbsoluteCoord = propertyValue("position", MyGUI::IntPoint());
        mAbsoluteCoord = propertyValue("size", MyGUI::IntSize());
        mRelativeCoord = propertyValue("relativePosition", MyGUI::FloatPoint());
        mRelativeCoord = propertyValue("relativeSize", MyGUI::FloatSize());
        mAnchor = propertyValue("anchor", MyGUI::FloatSize());
        mWidget->setVisible(propertyValue("visible", true));
        mWidget->setPointer(propertyValue("pointer", std::string("arrow")));
        mWidget->setAlpha(propertyValue("alpha", 1.f));
        mWidget->setInheritsAlpha(propertyValue("inheritAlpha", true));
    }

    void WidgetExtension::updateChildrenCoord()
    {
        for (WidgetExtension* w : mTemplateChildren)
            w->updateCoord();
        for (WidgetExtension* w : mChildren)
            w->updateCoord();
    }

    MyGUI::IntSize WidgetExtension::parentSize()
    {
        if (mParent && !mTemplateChild)
            return mParent->childScalingSize();
        else
            return widget()->getParentSize();
    }

    MyGUI::IntSize WidgetExtension::calculateSize()
    {
        MyGUI::IntSize pSize = parentSize();
        MyGUI::IntSize newSize;
        newSize = mAbsoluteCoord.size() + mForcedCoord.size();
        newSize.width += mRelativeCoord.width * pSize.width;
        newSize.height += mRelativeCoord.height * pSize.height;
        return newSize;
    }

    MyGUI::IntPoint WidgetExtension::calculatePosition(const MyGUI::IntSize& size)
    {
        MyGUI::IntSize pSize = parentSize();
        MyGUI::IntPoint newPosition;
        newPosition = mAbsoluteCoord.point() + mForcedCoord.point();
        newPosition.left += mRelativeCoord.left * pSize.width - mAnchor.width * size.width;
        newPosition.top += mRelativeCoord.top * pSize.height - mAnchor.height * size.height;
        return newPosition;
    }

    MyGUI::IntCoord WidgetExtension::calculateCoord()
    {
        MyGUI::IntCoord newCoord;
        newCoord = calculateSize();
        newCoord = calculatePosition(newCoord.size());
        return newCoord;
    }

    MyGUI::IntSize WidgetExtension::childScalingSize()
    {
        return mSlot->widget()->getSize();
    }

    void WidgetExtension::triggerEvent(std::string_view name, sol::object argument) const
    {
        auto it = mCallbacks.find(name);
        if (it != mCallbacks.end())
            it->second(argument, mLayout);
    }

    void WidgetExtension::keyPress(MyGUI::Widget*, MyGUI::KeyCode code, MyGUI::Char ch)
    {
        if (code == MyGUI::KeyCode::None)
        {
            propagateEvent("textInput", [ch](auto w) {
                MyGUI::UString uString;
                uString.push_back(static_cast<MyGUI::UString::unicode_char>(ch));
                return sol::make_object(w->lua(), uString.asUTF8());
            });
        }
        else
            propagateEvent("keyPress", [code](auto w){ return w->keyEvent(code); });
    }

    void WidgetExtension::keyRelease(MyGUI::Widget*, MyGUI::KeyCode code)
    {
        propagateEvent("keyRelease", [code](auto w) { return w->keyEvent(code); });
    }

    void WidgetExtension::mouseMove(MyGUI::Widget*, int left, int top)
    {
        propagateEvent("mouseMove", [left, top](auto w) { return w->mouseEvent(left, top); });
    }

    void WidgetExtension::mouseDrag(MyGUI::Widget*, int left, int top, MyGUI::MouseButton button)
    {
        propagateEvent("mouseMove", [left, top, button](auto w) { return w->mouseEvent(left, top, button); });
    }

    void WidgetExtension::mouseClick(MyGUI::Widget* _widget)
    {
        propagateEvent("mouseClick", [](auto){ return sol::nil; });
    }

    void WidgetExtension::mouseDoubleClick(MyGUI::Widget* _widget)
    {
        propagateEvent("mouseDoubleClick", [](auto){ return sol::nil; });
    }

    void WidgetExtension::mousePress(MyGUI::Widget*, int left, int top, MyGUI::MouseButton button)
    {
        propagateEvent("mousePress", [left, top, button](auto w) { return w->mouseEvent(left, top, button); });
    }

    void WidgetExtension::mouseRelease(MyGUI::Widget*, int left, int top, MyGUI::MouseButton button)
    {
        propagateEvent("mouseRelease", [left, top, button](auto w) { return w->mouseEvent(left, top, button); });
    }

    void WidgetExtension::focusGain(MyGUI::Widget*, MyGUI::Widget*)
    {
        propagateEvent("focusGain", [](auto){ return sol::nil; });
    }

    void WidgetExtension::focusLoss(MyGUI::Widget*, MyGUI::Widget*)
    {
        propagateEvent("focusLoss", [](auto){ return sol::nil; });
    }
}
