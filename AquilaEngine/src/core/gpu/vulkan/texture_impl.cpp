#include "texture_impl.h"
#include "device_impl.h"
#include "image_impl.h"

using namespace core::gpu;

Texture::Texture(const Device& _device, const Image& _image, utils::ETextureFilter _filter)
    : m_impl(new Impl)
{
    vk::PhysicalDeviceProperties properties = _device.GetImpl().physicalDevice.getProperties();

    vk::Filter vkFilter = (_filter == utils::ETextureFilter::Nearest) ? vk::Filter::eNearest : vk::Filter::eLinear;

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vkFilter;
    samplerInfo.minFilter = vkFilter;
    samplerInfo.mipmapMode = (_filter == utils::ETextureFilter::Nearest)
        ? vk::SamplerMipmapMode::eNearest
        : vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat; 
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = (_filter == utils::ETextureFilter::Nearest) ? vk::False : vk::True;
    samplerInfo.maxAnisotropy = (_filter == utils::ETextureFilter::Nearest) ? 1.0f : properties.limits.maxSamplerAnisotropy;
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