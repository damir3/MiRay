#pragma once

class INode;

std::vector<INode *> collectAllChildren(const std::vector<INode *> & nodes);
std::vector<INode *> sortAccordingToTheSceneTree(const std::vector<INode *> & src, INode * root, bool reverse);
