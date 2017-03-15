#include "fusepp_mocks.h"

namespace fusepp {

MockNodeHandle::MockNodeHandle(){}
MockNodeHandle::~MockNodeHandle(){}

MockFileHandle::MockFileHandle(){}
MockFileHandle::~MockFileHandle(){}

MockDirHandle::MockDirHandle(){}
MockDirHandle::~MockDirHandle(){}

MockNode::MockNode(path_t rel_path) : Node(rel_path) {}
MockNode::~MockNode(){}

MockMount::MockMount(){}
MockMount::~MockMount(){}

} //namespace fusepp
