#pragma once

#include "iscenegraph.h"
#include "iselection.h"
#include "iselectiontest.h"

namespace selection
{

// Base class for SelectionTesters, provides some convenience methods
class SelectionTestWalker :
	public scene::Graph::Walker
{
protected:
	Selector& _selector;
	SelectionTest& _test;

protected:
	SelectionTestWalker(Selector& selector, SelectionTest& test) :
		_selector(selector),
		_test(test)
	{}

	void printNodeName(const scene::INodePtr& node);

	// Returns non-NULL if the given node is an Entity
	scene::INodePtr getEntityNode(const scene::INodePtr& node);

	// Returns non-NULL if the given node's parent is a GroupNode
	scene::INodePtr getParentGroupEntity(const scene::INodePtr& node);

	// Returns true if the node is worldspawn
	bool entityIsWorldspawn(const scene::INodePtr& node);

	// Performs the actual selection test on the given node
	// The nodeToBeTested is the node that is tested against, whereas 
	// the selectableNode is the one that gets pushed to the Selector
	// These two might be the same node, but this is not mandatory.
	void performSelectionTest(const scene::INodePtr& selectableNode, const scene::INodePtr& nodeToBeTested);
};

// A Selector which is testing for entities. This successfully
// checks for selections of child primitives of func_* entities too.
class EntitySelector :
	public SelectionTestWalker
{
public:
	EntitySelector(Selector& selector, SelectionTest& test) :
		SelectionTestWalker(selector, test)
	{}

	bool visit(const scene::INodePtr& node);
};

// A Selector looking for worldspawn primitives only.
class PrimitiveSelector :
	public SelectionTestWalker
{
public:
	PrimitiveSelector(Selector& selector, SelectionTest& test) :
		SelectionTestWalker(selector, test)
	{}

	bool visit(const scene::INodePtr& node);
};

// A Selector looking for child primitives of group nodes only, non-worldspawn parent
class GroupChildPrimitiveSelector :
	public SelectionTestWalker
{
public:
	GroupChildPrimitiveSelector(Selector& selector, SelectionTest& test) :
		SelectionTestWalker(selector, test)
	{}

	bool visit(const scene::INodePtr& node);
};

// A selector testing for all kinds of selectable items, entities and primitives.
// Worldspawn primitives are selected directly, for child primitives of func_* ents
// the selection will be "relayed" to the parent entity.
class AnySelector :
	public SelectionTestWalker
{
public:
	AnySelector(Selector& selector, SelectionTest& test) :
		SelectionTestWalker(selector, test)
	{}

	bool visit(const scene::INodePtr& node);
};

// A class seeking for components, can be used either to traverse the
// selectionsystem or the scene graph as a whole.
class ComponentSelector :
	public SelectionTestWalker,
	public SelectionSystem::Visitor
{
private:
	SelectionSystem::EComponentMode _mode;

public:
	ComponentSelector(Selector& selector, SelectionTest& test,
		SelectionSystem::EComponentMode mode) :
		SelectionTestWalker(selector, test),
		_mode(mode)
	{}

	// scene::Graph::Walker implementation
	bool visit(const scene::INodePtr& node);

	// SelectionSystem::Visitor implementation
	void visit(const scene::INodePtr& node) const;

protected:
	void performComponentselectionTest(const scene::INodePtr& node) const;
};

}