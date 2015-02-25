// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "intrusive_list.h"
#include "gtest/gtest.h"

namespace pindrop {

class IntegerListNode {
 public:
  IntegerListNode(int value) : value_(value), node_() {}
  int value() const { return value_; }

  PINDROP_INTRUSIVE_GET_NODE(node_);
  PINDROP_INTRUSIVE_LIST_NODE_GET_CLASS(IntegerListNode, node_);

 private:
  int value_;
  IntrusiveListNode node_;
};

bool IntegerListNodeComparitor(const IntrusiveListNode& a,
                               const IntrusiveListNode& b) {
  const IntegerListNode* int_a = IntegerListNode::GetInstanceFromListNode(&a);
  const IntegerListNode* int_b = IntegerListNode::GetInstanceFromListNode(&b);
  return int_a->value() < int_b->value();
}

TEST(IntrusiveListSort, ForwardOrder) {
  IntrusiveListNode list;
  IntegerListNode one(1);
  IntegerListNode two(2);
  IntegerListNode three(3);
  IntegerListNode four(4);
  IntegerListNode five(5);
  list.InsertAfter(one.GetListNode());
  one.GetListNode()->InsertAfter(two.GetListNode());
  two.GetListNode()->InsertAfter(three.GetListNode());
  three.GetListNode()->InsertAfter(four.GetListNode());
  four.GetListNode()->InsertAfter(five.GetListNode());

  list.Sort(&IntegerListNodeComparitor);

  IntrusiveListNode* node = list.GetNext();
  EXPECT_EQ(1, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(2, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(3, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(4, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(5, IntegerListNode::GetInstanceFromListNode(node)->value());
}

TEST(IntrusiveListSort, BackwardOrder) {
  IntrusiveListNode list;
  IntegerListNode one(1);
  IntegerListNode two(2);
  IntegerListNode three(3);
  IntegerListNode four(4);
  IntegerListNode five(5);
  list.InsertAfter(five.GetListNode());
  five.GetListNode()->InsertAfter(four.GetListNode());
  four.GetListNode()->InsertAfter(three.GetListNode());
  three.GetListNode()->InsertAfter(two.GetListNode());
  two.GetListNode()->InsertAfter(one.GetListNode());

  list.Sort(&IntegerListNodeComparitor);

  IntrusiveListNode* node = list.GetNext();
  EXPECT_EQ(1, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(2, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(3, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(4, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(5, IntegerListNode::GetInstanceFromListNode(node)->value());
}

TEST(IntrusiveListSort, RandomOrder) {
  IntrusiveListNode list;
  IntegerListNode one(1);
  IntegerListNode two(2);
  IntegerListNode three(3);
  IntegerListNode four(4);
  IntegerListNode five(5);
  list.InsertAfter(two.GetListNode());
  two.GetListNode()->InsertAfter(four.GetListNode());
  four.GetListNode()->InsertAfter(five.GetListNode());
  five.GetListNode()->InsertAfter(one.GetListNode());
  one.GetListNode()->InsertAfter(three.GetListNode());

  list.Sort(&IntegerListNodeComparitor);

  IntrusiveListNode* node = list.GetNext();
  EXPECT_EQ(1, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(2, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(3, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(4, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(5, IntegerListNode::GetInstanceFromListNode(node)->value());
}

TEST(IntrusiveListSort, ShortList) {
  IntrusiveListNode list;
  IntegerListNode one(1);
  IntegerListNode two(2);
  list.InsertAfter(two.GetListNode());
  two.GetListNode()->InsertAfter(one.GetListNode());

  list.Sort(&IntegerListNodeComparitor);

  IntrusiveListNode* node = list.GetNext();
  EXPECT_EQ(1, IntegerListNode::GetInstanceFromListNode(node)->value());
  node = node->GetNext();
  EXPECT_EQ(2, IntegerListNode::GetInstanceFromListNode(node)->value());
}

class IntegerItem : public TypedIntrusiveListNode<IntegerItem> {
 public:
  IntegerItem(int value) : value_(value) {}
  int value() const { return value_; }

 private:
  int value_;
};

bool IntegerItemComparitor(const IntegerItem& a, const IntegerItem& b) {
  return a.value() < b.value();
}

TEST(TypedIntrusiveListSort, ForwardOrder) {
  TypedIntrusiveListNode<IntegerItem> list;
  IntegerItem one(1);
  IntegerItem two(2);
  IntegerItem three(3);
  IntegerItem four(4);
  IntegerItem five(5);
  list.InsertAfter(&one);
  one.InsertAfter(&two);
  two.InsertAfter(&three);
  three.InsertAfter(&four);
  four.InsertAfter(&five);

  list.Sort(&IntegerItemComparitor);

  IntegerItem* item = list.GetNext();
  EXPECT_EQ(1, item->value());
  item = item->GetNext();
  EXPECT_EQ(2, item->value());
  item = item->GetNext();
  EXPECT_EQ(3, item->value());
  item = item->GetNext();
  EXPECT_EQ(4, item->value());
  item = item->GetNext();
  EXPECT_EQ(5, item->value());
}

TEST(TypedIntrusiveListSort, BackwardOrder) {
  TypedIntrusiveListNode<IntegerItem> list;
  IntegerItem one(1);
  IntegerItem two(2);
  IntegerItem three(3);
  IntegerItem four(4);
  IntegerItem five(5);
  list.InsertAfter(&five);
  five.InsertAfter(&four);
  four.InsertAfter(&three);
  three.InsertAfter(&two);
  two.InsertAfter(&one);

  list.Sort(&IntegerItemComparitor);

  IntegerItem* item = list.GetNext();
  EXPECT_EQ(1, item->value());
  item = item->GetNext();
  EXPECT_EQ(2, item->value());
  item = item->GetNext();
  EXPECT_EQ(3, item->value());
  item = item->GetNext();
  EXPECT_EQ(4, item->value());
  item = item->GetNext();
  EXPECT_EQ(5, item->value());
}

TEST(TypedIntrusiveListSort, RandomOrder) {
  TypedIntrusiveListNode<IntegerItem> list;
  IntegerItem one(1);
  IntegerItem two(2);
  IntegerItem three(3);
  IntegerItem four(4);
  IntegerItem five(5);
  list.InsertAfter(&two);
  two.InsertAfter(&four);
  four.InsertAfter(&five);
  five.InsertAfter(&one);
  one.InsertAfter(&three);

  list.Sort(&IntegerItemComparitor);

  IntegerItem* item = list.GetNext();
  EXPECT_EQ(1, item->value());
  item = item->GetNext();
  EXPECT_EQ(2, item->value());
  item = item->GetNext();
  EXPECT_EQ(3, item->value());
  item = item->GetNext();
  EXPECT_EQ(4, item->value());
  item = item->GetNext();
  EXPECT_EQ(5, item->value());
}

TEST(TypedIntrusiveListSort, ShortList) {
  TypedIntrusiveListNode<IntegerItem> list;
  IntegerItem one(1);
  IntegerItem two(2);
  list.InsertAfter(&two);
  two.InsertAfter(&one);

  list.Sort(&IntegerItemComparitor);

  IntegerItem* item = list.GetNext();
  EXPECT_EQ(1, item->value());
  item = item->GetNext();
  EXPECT_EQ(2, item->value());
}

}  // namespace pindrop

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

