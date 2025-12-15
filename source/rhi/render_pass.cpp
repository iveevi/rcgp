#include "rhi/render_pass.hpp"

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
	return vk::AttachmentReference()
		.setAttachment(idx)
		.setLayout(layout);
}
