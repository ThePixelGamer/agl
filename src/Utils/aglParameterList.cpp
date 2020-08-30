#include "agl/Utils/aglParameterList.h"
#include <basis/seadRawPrint.h>
#include <prim/seadFormatPrint.h>
#include <prim/seadSafeString.h>
#include "agl/Utils/aglParameter.h"
#include "agl/Utils/aglParameterObj.h"
#include "agl/Utils/aglParameterStringMgr.h"

namespace agl::utl {

IParameterList::IParameterList() {
    setParameterListName_(sead::SafeString::cEmptyString);
}

void IParameterList::setParameterListName_(const sead::SafeString& name) {
#ifdef SEAD_DEBUG
    if (ParameterStringMgr::instance())
        mName = ParameterStringMgr::instance()->appendString(name);
    else
        mName = nullptr;
#endif

    mNameHash = ParameterBase::calcHash(name);
}

IParameterList::~IParameterList() {
    ;
}

void IParameterList::addList(IParameterList* child, const sead::SafeString& name) {
    SEAD_ASSERT(child != nullptr);
    child->setParameterListName_(name);

    (!mpChildListTail ? mpChildListHead : mpChildListTail->mNext) = child;
    mpChildListTail = child;
    child->mParent = this;
}

void IParameterList::addObj(IParameterObj* child, const sead::SafeString& name) {
    SEAD_ASSERT(child != nullptr);

#ifdef SEAD_DEBUG
    if (ParameterStringMgr::instance())
        child->mName = ParameterStringMgr::instance()->appendString(name);
#endif
    child->mNameHash = ParameterBase::calcHash(name);

    (!mpChildObjTail ? mpChildObjHead : mpChildObjTail->mNext) = child;
    mpChildObjTail = child;
}

void IParameterList::clearList() {
    for (auto* i = mpChildListHead; i;) {
        auto* next = i->mNext;
        i->mNext = nullptr;
        i = next;
    }
    mpChildListHead = nullptr;
    mpChildListTail = nullptr;
}

void IParameterList::clearObj() {
    for (auto* i = mpChildObjHead; i;) {
        auto* next = i->mNext;
        i->mNext = nullptr;
        i = next;
    }
    mpChildObjHead = nullptr;
    mpChildObjTail = nullptr;
}

void IParameterList::removeList(IParameterList* child) {
    SEAD_ASSERT(child != nullptr);
    auto* i = mpChildListHead;
    if (!i)
        return;

    IParameterList* prev = nullptr;
    while (true) {
        if (i == child)
            break;
        prev = i;
        i = i->mNext;
        if (!i)
            return;
    }

    (prev ? prev->mNext : mpChildListHead) = child->mNext;
    if (!child->mNext) {
        SEAD_ASSERT(mpChildListTail == child);
        mpChildListTail = prev;
    }
    child->mNext = nullptr;
}

void IParameterList::removeObj(IParameterObj* child) {
    SEAD_ASSERT(child != nullptr);
    auto* i = mpChildObjHead;
    if (!i)
        return;

    IParameterObj* prev = nullptr;
    while (true) {
        if (i == child)
            break;
        prev = i;
        i = i->mNext;
        if (!i)
            return;
    }

    (prev ? prev->mNext : mpChildObjHead) = child->mNext;
    if (!child->mNext) {
        SEAD_ASSERT(mpChildObjTail == child);
        mpChildObjTail = prev;
    }
    child->mNext = nullptr;
}

void IParameterList::applyResParameterList(ResParameterList list) {
    return applyResParameterList_(false, list, {}, 0.0);
}

void IParameterList::applyResParameterList(ResParameterList list1, ResParameterList list2, f32 t) {
    if (list1.ptr() && t <= 0.0)
        return applyResParameterList_(false, list1, {}, 0.0);
    if (list2.ptr() && t >= 1.0)
        return applyResParameterList_(false, list2, {}, 0.0);
    return applyResParameterList_(true, list1, list2, t);
}

bool IParameterList::isComplete(ResParameterList res, bool) const {
    if (!res.ptr())
        return false;

    s32 obj_count = 0;
    for (auto* i = mpChildObjHead; i; i = i->mNext) {
        if (res.searchObjIndex(i->mNameHash) == -1)
            return false;
        ++obj_count;
    }

    s32 list_count = 0;
    for (auto* i = mpChildListHead; i; i = i->mNext) {
        if (res.searchListIndex(i->mNameHash) == -1)
            return false;
        ++list_count;
    }

    return obj_count == res.getResParameterObjNum() && list_count == res.getResParameterListNum();
}

sead::SafeString IParameterList::getName() const {
    return mName;
}

const char* IParameterList::getTagName() {
    return "param_list";
}

bool IParameterList::verify() const {
    bool ok = true;
    ok &= verifyList();
    ok &= verifyObj();
    for (auto* i = mpChildListHead; i; i = i->mNext)
        ok &= i->verify();
    for (auto* i = mpChildObjHead; i; i = i->mNext)
        ok &= i->verify();
    return ok;
}

bool IParameterList::verifyList() const {
    bool ret = true;
    for (auto* i = mpChildListHead; i; i = i->mNext)
        ret &= verifyList(i, i->mNext);
    return ret;
}

bool IParameterList::verifyObj() const {
    bool ret = true;
    for (auto* i = mpChildObjHead; i; i = i->mNext)
        ret &= verifyObj(i, i->mNext);
    return ret;
}

bool IParameterList::verifyList(IParameterList* p_check, IParameterList* other) const {
    SEAD_ASSERT(p_check != nullptr);
    auto* list = other;
    bool ok = true;
    while (list) {
        if (p_check->getNameHash() == list->getNameHash()) {
            sead::BufferingPrintFormatter ss;
            ss << "Same hash code at [%s] and [%s]. Please change.\n"
               << p_check->getName().cstr() << list->getName().cstr() << sead::flush;
            ok = false;
        }
        list = list->mNext;
    }
    return ok;
}

bool IParameterList::verifyObj(IParameterObj* p_check, IParameterObj* other) const {
    SEAD_ASSERT(p_check != nullptr);
    auto* list = other;
    bool ok = true;
    while (list) {
        if (p_check->getNameHash() == list->getNameHash()) {
            sead::BufferingPrintFormatter ss;
            ss << "Same hash code at [%s] and [%s]. Please change.\n"
               << p_check->getName().cstr() << list->getName().cstr() << sead::flush;
            ok = false;
        }
        list = list->mNext;
    }
    return ok;
}

// NON_MATCHING: the useless ternary gets optimized out (n cannot be 0 when it is reached)
ResParameterObj IParameterList::searchResParameterObj_(ResParameterList res,
                                                       const IParameterObj& obj) const {
    if (!res.ptr())
        return {};

    const auto n = res.getResParameterObjNum();
    auto ret = n != 0 ? res.getResParameterObj() : ResParameterObj();
    for (s32 i = 0; i != n; ++i) {
        if (obj.isApply_(ret))
            return ret;
        ++ret.mPtr;
    }
    return {};
}

IParameterObj* IParameterList::searchChildParameterObj_(ResParameterObj res,
                                                        IParameterObj* obj) const {
    if (!res.ptr() || !mpChildObjHead)
        return nullptr;

    auto* start = obj ? obj : mpChildObjHead;
    auto* child = start;
    while (!child->isApply_(res)) {
        child = child->mNext;
        if (!child)
            child = mpChildObjHead;

        if (child == start)
            return nullptr;
    }
    return child;
}

// NON_MATCHING: same issue with the ternary
ResParameterList IParameterList::searchResParameterList_(ResParameterList res,
                                                         const IParameterList& list) const {
    if (!res.ptr())
        return {};

    const auto n = res.getResParameterListNum();
    auto ret = n != 0 ? res.getResParameterList() : ResParameterList();
    for (s32 i = 0; i != n; ++i) {
        if (list.isApply_(ret))
            return ret;
        ++ret.mPtr;
    }
    return {};
}

IParameterList* IParameterList::searchChildParameterList_(ResParameterList res) const {
    if (!res.ptr())
        return nullptr;

    for (auto* child = mpChildListHead; child; child = child->mNext) {
        if (child->isApply_(res))
            return child;
    }
    return nullptr;
}

// NON_MATCHING: the ternary, again...
void IParameterList::applyResParameterObjB_(bool interpolate, ResParameterList res, f32 t) {
    if (!res.ptr())
        return;

    const auto n = res.getResParameterObjNum();
    auto r = n != 0 ? res.getResParameterObj() : ResParameterObj();
    IParameterObj* obj = nullptr;
    for (s32 i = 0; i != n; ++i, ++r.mPtr) {
        auto* result = searchChildParameterObj_(r, obj);
        if (result) {
            result->applyResParameterObj_(interpolate, {}, r, t, this);
            obj = result;
        }
    }
}

// NON_MATCHING: the ternary, again...
[[gnu::noinline]] void IParameterList::applyResParameterListB_(bool interpolate,
                                                               ResParameterList res, f32 t) {
    if (!res.ptr())
        return;

    const auto n = res.getResParameterListNum();
    auto r = n != 0 ? res.getResParameterList() : ResParameterList();
    for (s32 i = 0; i != n; ++i, ++r.mPtr) {
        auto* list = searchChildParameterList_(r);
        if (list)
            list->applyResParameterList_(interpolate, {}, r, t);
    }
}

// WIP
void IParameterList::applyResParameterList_(bool interpolate, ResParameterList l1,
                                            ResParameterList l2, f32 t) {
    if (!preRead_())
        return;

    // Recursively apply all parameter objects.
    if (l1.ptr()) {
        const auto n = l1.getResParameterObjNum();
        auto r = n != 0 ? l1.getResParameterObj() : ResParameterObj();
        IParameterObj* obj = nullptr;
        for (s32 i = 0; i != n; ++i, ++r.mPtr) {
            auto* child = searchChildParameterObj_(r, obj);
            if (child) {
                const auto obj2 = searchResParameterObj_(l2, *child);
                child->applyResParameterObj_(interpolate, r, obj2, t, this);
                obj = child;
            } else {
                applyResParameterObjB_(interpolate, l2, t);
            }
        }
    } else {
        applyResParameterObjB_(interpolate, l2, t);
    }

    // Now recursively apply all parameter lists.
    if (l1.ptr()) {
        const auto n = l1.getResParameterListNum();
        auto r = n != 0 ? l1.getResParameterList() : ResParameterList();
        for (s32 i = 0; i != n; ++i, ++r.mPtr) {
            auto* child = searchChildParameterList_(r);
            if (l2.ptr()) {
                if (child) {
                    const auto other_list = searchResParameterList_(l2, *child);
                    child->applyResParameterList_(interpolate, r, other_list, t);
                } else {
                    applyResParameterListB_(interpolate, l2, t);
                }
            } else {
                if (child)
                    applyResParameterList_(interpolate, r, {}, t);
                else
                    applyResParameterListB_(interpolate, {}, t);
            }
        }
    } else {
        applyResParameterListB_(interpolate, l2, t);
    }

    postRead_();
}

}  // namespace agl::utl