#include "texture_impl.h"
#include "device_impl.h"
#include "image_impl.h"

using namespace core::gpu;

Texture::Texture(const Device& _device, const Image& _image)
    : m_impl(new Impl)
{
    vk::PhysicalDeviceProperties properties = _device.GetImpl().physicalDevice.getProperties();

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = vk::True;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    m_impl->image = &_image;
    m_impl->sampler = vk::raii::Sampler(_device.GetImpl().device, samplerInfo);
}

Texture::~Texture() = default;

Texture::Impl& Texture::GetImpl() const
{
    return *m_impl;
}