#include "rhi/render_pass.hpp"

namespace rcgp {

VkAttachmentDescription &Attachments::operator[](const std::string &key) &
{
	if (mapping.contains(key))
		return descriptions[mapping[key]];

	mapping[key] = descriptions.size();
	descriptions.emplace_back();

	return descriptions.back();
}

VkAttachmentReference Attachments::reference(const std::string &key, VkImageLayout layout) const
{
	VkAttachmentReference result {};
	result.attachment = mapping.at(key);
	result.layout = layout;
	return result;
}

} // namespace rcgp
