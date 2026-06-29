
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace EditorExtensions
{
    class EditorExtensionsRequests
    {
    public:
        AZ_RTTI(EditorExtensionsRequests, "{D46B642F-FCA7-402E-AD95-C9B4BE9761E3}");
        virtual ~EditorExtensionsRequests() = default;
        // Put your public methods here
    };
    
    class EditorExtensionsBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using EditorExtensionsRequestBus = AZ::EBus<EditorExtensionsRequests, EditorExtensionsBusTraits>;
    using EditorExtensionsInterface = AZ::Interface<EditorExtensionsRequests>;

} // namespace EditorExtensions
