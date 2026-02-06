

#ifndef MJView_hpp
#define MJView_hpp

#include <stdio.h>
#include <vector>
#include <set>
#include "MathUtils.h"
#include "WindowInfo.h"
#include "Vulkan.h"

#define MAX_MOUSE_BUTTONS 2

class MJCache;
class GCommandBuffer;
class MJRenderTarget;
class MJDrawQuad;
class TuiTable;
class TuiRef;
class TuiFunction;

enum  {
    MJPositionOuterLeft,
    MJPositionInnerLeft,
    MJPositionCenter,
    MJPositionInnerRight,
    MJPositionOuterRight,
    MJPositionBelow,
    MJPositionBottom,
    MJPositionTop,
    MJPositionAbove,
};

struct  MJViewPosition {
    int h;
    int v;
    
    MJViewPosition() :
    h(MJPositionCenter),
    v(MJPositionCenter) {};
    
    MJViewPosition(int h, int v) :
    h(h),
    v(v) {};
};

class MJView {
public:
    MJCache* cache;
    WindowInfo* windowInfo;
    
    TuiTable* stateTable = nullptr;
    TuiTable* rootTable = nullptr;
    
    MJView* parentView;
    std::vector<MJView*> subviews;
    
    std::map<std::string, MJView*>* viewsByID = nullptr;
    bool isTopLevel = false;
    std::string relativeViewToLoadID;

    float alpha;
    bool masksEvents = false;
	MJRenderPass currentRenderPass;

	dmat4 combinedRenderMatrixWithoutAdditionalMatrix = dmat4(1.0);
	dmat4 additionalMatrix = dmat4(1.0);
	bool hasAdditionalMatrix = false;
    
    dmat4 baseRenderMatrix = dmat4(1.0);
	dmat4 combinedRenderMatrix = dmat4(1.0);
	dmat4 clipMatrix = dmat4(1.0);

	dmat3 rotation = dmat3(1.0);
	dmat3 windowRotation = dmat3(1.0);
	dvec3 windowPosition = dvec3(0.0,0.0,0.0);

    double renderScale;
	double windowZ = 0.0;
	double windowZRenderOffset = 0.0;
	dvec2 size;

	bool renderXRay = false;
	bool allowViewMovementUnderCursorToTriggerHoverStartEvents = false;

    bool mouseInside;
    bool mouseDownWasInside[MAX_MOUSE_BUTTONS];
    bool sendClickEventOnMouseUpInside[MAX_MOUSE_BUTTONS];
    bool receivedMouseDown[MAX_MOUSE_BUTTONS];

	bool debugRenderFlag = false;

	bool disableClipping = false;
    bool depthTestEnabled = false;

	bool isWorldView = false;
	bool mouseHoverDirty = false;

	dvec2 mouseDownLocalPoint;
	bool dragDistanceAboveClickOutsideThreshold;
    
    std::string idString;
    
    static MJView* loadUnknownViewFromTable(TuiTable* subViewTable, MJView* parentView, bool isRoot);
    /*

	LuaRef getwasRemovedLuaFunction(lua_State* luaState) const GET_MJ_LUA_DEFINITION(wasRemovedLuaFunction)
	void setwasRemovedLuaFunction(LuaRef ref);

    
    LuaRef getkeyChangedLuaFunction(lua_State* luaState) const GET_MJ_LUA_DEFINITION(keyChangedLuaFunction)
    void setkeyChangedLuaFunction(LuaRef ref) SET_MJ_LUA_DEFINITION(keyChangedLuaFunction)

    
    LuaRef getuserData(lua_State* luaState) const GET_MJ_LUA_DEFINITION(userData)
    void setuserData(LuaRef ref) SET_MJ_LUA_DEFINITION(userData)*/
    
    //void setUpdateFunction(std::function<void(double)> updateFunction_);
    
    TuiFunction* updateFunction = nullptr;
    
    std::function<bool(bool, int, int, bool)> keyChangedFunction;
    
    /*std::function<void(dvec2)> hoverStartFunction;
    std::function<void(dvec2)> hoverMovedFunction;
    std::function<void(void)> hoverEndFunction;
    std::function<void(bool)> hiddenStateChangedFunction;*/
    
    TuiFunction* parentSizeChangedFunction = nullptr;
    
    TuiFunction* hoverStartFunction = nullptr;
    TuiFunction* hoverMovedFunction = nullptr;
    TuiFunction* hoverEndFunction = nullptr;
    TuiFunction* hiddenStateChangedFunction = nullptr;
    
    //std::function<void(int, dvec2)> mouseDownFunction;
    //std::function<void(int, dvec2)> mouseUpFunction;
    //std::function<void(dvec2)> mouseDraggedFunction;
    //std::function<void(dvec2, dvec2)> mouseWheelFunction;
    TuiFunction* mouseDownFunction = nullptr;
    TuiFunction* mouseUpFunction = nullptr;
    TuiFunction* mouseDraggedFunction = nullptr;
    TuiFunction* mouseWheelFunction = nullptr;
    
    //std::function<void(dvec2)> clickFunction;
    //std::function<void(dvec2)> rightClickFunction;
    //std::function<void(int, dvec2)> clickOutsideFunction;
    //std::function<void(int, dvec2)> clickDownOutsideFunction;
    
    TuiFunction* clickFunction = nullptr;
    TuiFunction* rightClickFunction = nullptr;
    TuiFunction* clickOutsideFunction = nullptr;
    TuiFunction* clickDownOutsideFunction = nullptr;
    
    MJView* getViewWithID(const std::string& idString_) {
        if(!viewsByID || viewsByID->count(idString_) == 0)
        {
            return nullptr;
        }
        return viewsByID->at(idString_);
    }
    
    MJView* getSubViewWithID(const std::string& idString_) {
        for(MJView* subView : subviews)
        {
            if(subView->idString == idString_)
            {
                return subView;
            }
            MJView* foundSubview = subView->getSubViewWithID(idString_);
            if(foundSubview)
            {
                return foundSubview;
            }
        }
        
        return nullptr;
    }
    
    void setID(const std::string& idString_) {
        if(!idString.empty())
        {
            (*viewsByID).erase(idString);
        }
        idString = idString_;
        if(!idString.empty())
        {
            (*viewsByID)[idString] = this;
        }
    }

	void setDisableClipping(bool disableClipping_);
    
public:
    
    MJView(WindowInfo* windowInfo, MJCache* cache_);
    MJView(MJView* parentView_);
    virtual ~MJView();
    
    virtual std::string getDescription();
    
    dvec2 getSize() const;
    virtual void setSize(dvec2 size_);

    double getScale() const;
    virtual void setScale(double scale_);

	void setAdditionalMatrix(dmat4 additionalMatrix_);
    
    MJView* getRelativeView() const;
    void setRelativeView(MJView* relativeView);
    
    MJViewPosition getRelativePosition() const;
    void setRelativePosition(MJViewPosition relativePosition);
    
    void setRelativePosition(std::string alignmentX, std::string alignmentY); //convenenience when loading from tables
    
    dvec3 getBaseOffset() const;
    void setBaseOffset(dvec3 baseOffset_);

	dvec3 getAdditionalOffset() const;
	void setAdditionalOffset(dvec3 additionalOffset_);

	dmat3 getRotation() const;
	void setRotation(dmat3 rotation_);

	double getAlpha() const;
	void setAlpha(double alpha_);
    bool getRenderXRay() const;
    void setRenderXRay(bool renderXRay_);
    
    bool getHidden() const;
    void setHidden(bool hidden_);
    
    virtual void setDepthTestEnabled(bool depthTestEnabled_);
    
    void addSubview(MJView* subView);
    void removeSubview(MJView* subView);
    void removeFromParentView();
    void removeAllSubviews();
    
    void orderBack(MJView* subView);
    void orderFront(MJView* subView);

    virtual void update(float dt);
    virtual void preRender(GCommandBuffer* commandBuffer, MJRenderPass renderPass, int renderTargetCompatibilityIndex, double dt, double frameLerp, double animationTimer);
    virtual void render(GCommandBuffer* commandBuffer);
	virtual void renderSelf(GCommandBuffer* commandBuffer);

	virtual void updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos);
    
    void addDependentView(MJView* dependentView);
    void removeDependentView(MJView* dependentView);
    
    dvec2 windowPointToLocal(dvec3 windowRayStart, dvec3 windowRayDirection);
	dvec2 localPointToWindow(dvec3 localPoint);
    virtual bool containsPoint(dvec3 windowRayStart, dvec3 windowRayDirection);



	virtual bool visibleUIContainsPointIncludingSubViews3D(dvec3 windowRayStart, dvec3 windowRayDirection);
	virtual bool mouseMoved3D(dvec3 windowRayStart, dvec3 windowRayDirection, bool obstructed, bool triggeredByUIMovement);
	virtual bool mouseDown3D(dvec3 windowRayStart, dvec3 windowRayDirection, bool obstructed, int buttonIndex);
	virtual bool mouseUp3D(dvec3 windowRayStart, dvec3 windowRayDirection, int buttonIndex);
	virtual bool mouseWheel3D(dvec3 windowRayStart, dvec3 windowRayDirection, dvec2 scrollChange);

	virtual bool visibleUIContainsPointIncludingSubViews(dvec2 mousePos);
    virtual bool mouseMoved(dvec2 mousePos, bool obstructed = false);
    virtual bool mouseDown(dvec2 mousePos, int buttonIndex, bool obstructed = false);
    virtual bool mouseUp(dvec2 mousePos, int buttonIndex);

    virtual bool keyChanged(bool isDown, int code, int modKey, bool isRepeat);
    
    void cleanupRemovedSubviews(); //it isn't safe to delete subviews on removal, as that could happen within a subview's event. So this needs to be called at some point later, probably at the root view level.

	void setCircleHitRadius(double radius);
	void removeCircleHitRadius ();

	void setWindowZOffset(double windowZOffset);
	double getWindowZOffset() const;

	void setWindowRotation(dmat3 windowRotation_);
	dmat3 getWindowRotation() const;

	void setWindowPosition(dvec3 windowPosition_);
	dvec3 getWindowPosition() const;

    dvec2 locationRelativeToView(dvec2 localPoint, MJView* otherView);

	virtual bool getIntersection(dvec3 rayStart, dvec3 rayDirection, double* distance);


	void setClipChildren(bool clipChildren_);
    void parentHiddenChanged(bool hidden_);
    
    void resetAnimationTimer();
    
    void loadFromFile(std::string filePath, TuiTable* parentTable);
    virtual void loadFromTable(TuiTable* table, bool isRoot = false); //isRoot used to prevent doRelativeViewLayoutsForTablePostLoad from being called recursively
    
    virtual void tableKeyChanged(const std::string& key, TuiRef* value);
    
    
    void doRelativeViewLayoutsForTablePostLoad();
    
protected:
    virtual void initInternals();
    
    void updateMatrix(int sanityCheck = 0);
    void addedToParentView(MJView* parentView_);

	virtual void reccursivelySetClipParent(MJView* clippingParent_);

    void updateRenderScale();

    
    virtual void invalidate();

	virtual void destroyDrawables();
	virtual void createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex);

	
	void childHasUpdateFunctionChanged(bool childHasUpdateFunction);

    
protected:
    
    MJViewPosition relativePosition;
    MJView* relativeView;
    

    dvec3 baseOffset = dvec3(0.0);
	dvec3 additionalOffset = dvec3(0.0);

	bool validMatrix = false;
    
    dvec2 origin;
    double scale;
    bool hidden;
	double circleHitRadius2;
	bool useCircleHitRadius = false;

	bool clipChildren = false;
	MJView* clippingParent = nullptr;

	bool hasUpdateFunctionOrChildWithUpdateFunction = false;
    
    float test;
    
    std::set<MJView*> dependentViews;
    
    std::set<MJView*> oldSubviewsToDelete;
    
    bool invalidated;
    
    double animationTimer = 0.0;
    double animationTimerOffset = 0.0;
    bool needsAnimationTimerReset = false;
    

};

#endif /* MJView_h */
