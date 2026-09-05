#include "NodeUtils.h"
#include "../../Shared/Interfaces/Scene.h"

namespace {

void buildListOfChildren(INode *node, std::vector<INode *> &children)
{
	if (std::find(children.begin(), children.end(), node) == children.end()) {
		children.push_back(node);
	}

	for (size_t i = 0; i < node->numChildren(SceneElement_All); i++) {
		auto elem = node->child(i, SceneElement_All);
		if (elem->type() != SceneElement_Geometry) {
			buildListOfChildren(static_cast<INode *>(elem), children);
		}
	}
}

} // namespace

std::vector<INode *> collectAllChildren(const std::vector<INode *> &nodes)
{
	std::vector<INode *> res;
	for (auto p : nodes) {
		buildListOfChildren(p, res);
	}
	return res;
}

std::vector<INode *> sortAccordingToTheSceneTree(const std::vector<INode *> &src, INode *root, bool reverse)
{
	if (src.size() < 2)
		return src;

	class NodeListSorter
	{
		std::vector<INode *> m_src, m_dst;
	public:
		NodeListSorter(const std::vector<INode *> &src) : m_src(src) {}
		const std::vector<INode *> &dst() const { return m_dst; }

		void sort(INode *node)
		{
			auto it = std::find(m_src.begin(), m_src.end(), node);
			if (it != m_src.end()) {
				m_dst.push_back(node);
				m_src.erase(it);
			}
			for (size_t i = 0; i < node->numChildren(SceneElement_All); i++) {
				auto elem = node->child(i, SceneElement_All);
				if (elem->type() != SceneElement_Geometry) {
					sort(static_cast<INode *>(elem));
				}
			}
		}
	};

	NodeListSorter sorter(src);
	sorter.sort(root);
	auto result = sorter.dst();
	if (reverse)
		std::reverse(result.begin(), result.end());

	return result;
}
