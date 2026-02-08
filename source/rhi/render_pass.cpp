#include "rhi/render_pass.hpp"

namespace rcgp {

vk::AttachmentDescription &Attachments::operator[](const std::string &key) &
{
	if (mapping.contains(key))
		return descriptions[mapping[key]];

	mapping[key] = descriptions.size();
	descriptions.emplace_back();

	return descriptions.back();
}

vk::AttachmentReference Attachments::reference(const std::string &key, vk::ImageLayout layout) const
{
	auto idx = mapping.at(key);
	auto result = vk::AttachmentReference();
	result.attachment = idx;
	result.layout = layout;
	return result;
}

} // namespace rcgp
