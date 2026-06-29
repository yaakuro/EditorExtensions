
#include "EditorExtensionsSystemComponent.h"

#include <EditorExtensions/EditorExtensionsTypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace EditorExtensions
{
    AZ_COMPONENT_IMPL(EditorExtensionsSystemComponent, "EditorExtensionsSystemComponent",
        EditorExtensionsSystemComponentTypeId);

    void EditorExtensionsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorExtensionsSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void EditorExtensionsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("EditorExtensionsService"));
    }

    void EditorExtensionsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("EditorExtensionsService"));
    }

    void EditorExtensionsSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void EditorExtensionsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    EditorExtensionsSystemComponent::EditorExtensionsSystemComponent()
    {
        if (EditorExtensionsInterface::Get() == nullptr)
        {
            EditorExtensionsInterface::Register(this);
        }
    }

    EditorExtensionsSystemComponent::~EditorExtensionsSystemComponent()
    {
        if (EditorExtensionsInterface::Get() == this)
        {
            EditorExtensionsInterface::Unregister(this);
        }
    }

    void EditorExtensionsSystemComponent::Init()
    {
    }

    void EditorExtensionsSystemComponent::Activate()
    {
        EditorExtensionsRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void EditorExtensionsSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        EditorExtensionsRequestBus::Handler::BusDisconnect();
    }

    void EditorExtensionsSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace EditorExtensions
