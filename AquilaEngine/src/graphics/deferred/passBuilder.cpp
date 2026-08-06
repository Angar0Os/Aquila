#include <graphics/deferred/passBuilder.h>
#include <graphics/deferred/renderGraph.h>

using namespace graphics::deferred;

PassAttachment& PassBuilder::CreateColorAttachment(const std::string& _name, core::gpu::utils::ETextureFormat _format)
{
	auto attachment = std::make_unique<PassAttachment>();

	attachment->name = _name;
	attachment->type = EAttachmentType::Color;
	attachment->format = _format;
	attachment->usage = core::gpu::utils::EImageUsage::ColorAttachment | core::gpu::utils::EImageUsage::Sampled;

	PassAttachment* ptr = attachment.get();

	m_graph.attachments.push_back(std::move(attachment));
	m_pass.outputs.push_back(ptr);
}

PassAttachment& PassBuilder::CreateDepthAttachment(const std::string& _name, core::gpu::utils::ETextureFormat _format)
{
	auto attachment = std::make_unique<PassAttachment>();

	attachment->name = _name;
	attachment->type = EAttachmentType::Depth;
	attachment->format = _format;
	attachment->usage = core::gpu::utils::EImageUsage::DepthStencilAttachment | core::gpu::utils::EImageUsage::Sampled;

	PassAttachment* ptr = attachment.get();

	m_graph.attachments.push_back(std::move(attachment));
	m_pass.outputs.push_back(ptr);
}

void PassBuilder::Read(PassAttachment& _attachment)
{
	m_pass.inputs.push_back(&_attachment);
}

void PassBuilder::Write(PassAttachment& _attachment)
{
	m_pass.outputs.push_back(&_attachment);
}