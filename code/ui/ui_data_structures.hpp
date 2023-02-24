#pragma once

#include "asset/types/script.hpp"
#include "core/lua.hpp"
#include "ui/ui_element.hpp"
#include <bitset>

namespace PG::UI
{

    template <typename T, typename IndexType, IndexType NULL_VAL, uint32_t MAX_SIZE>
    class StaticArrayAllocator
    {
    public:
        StaticArrayAllocator()
        {
            Clear();
        }

        void Clear()
        {
            elementCount = 0;
            nextFreeSlot = 0;
            slotsInUse.reset();
        }

        void FreeElement( IndexType idx )
        {
            PG_ASSERT( slotsInUse[idx], "Trying to free a slot that is already freed/unallocated" );
            slotsInUse[idx] = false;
            --elementCount;
            nextFreeSlot = std::min( nextFreeSlot, idx );
        }

        IndexType AllocOne()
        {
            PG_ASSERT( elementCount < MAX_SIZE && nextFreeSlot < MAX_SIZE );
            IndexType slot = nextFreeSlot;
            slotsInUse[slot] = true;
            ++elementCount;
            nextFreeSlot = FindNextFreeSlot( nextFreeSlot );
            return slot;
        }

        IndexType AllocMultiple( uint32_t numElements )
        {
            if ( elementCount + numElements > MAX_SIZE )
                return NULL_VAL;

            uint32_t count = 1;
            uint32_t startSlot = nextFreeSlot;
            while ( count < numElements && (startSlot + count) < MAX_SIZE )
            {
                if ( !slotsInUse[startSlot + count] )
                {
                    ++count;
                }
                else
                {
                    count = 1;
                    startSlot = FindNextFreeSlot( startSlot + count );
                }
            }

            if ( startSlot < MAX_SIZE )
            {
                for ( uint32_t i = 0; i < numElements; ++i )
                {
                    slotsInUse[startSlot + i] = true;
                }

                nextFreeSlot = FindNextFreeSlot( startSlot + numElements - 1 );
            }

            return startSlot;
        }

        IndexType Size() const { return (IndexType)elementCount; }
        bool IsSlotInUse( IndexType idx ) const { return slotsInUse[idx]; }
        T& operator[]( IndexType idx ) { return elements[idx]; }
        const T& operator[]( IndexType idx ) const { return elements[idx]; }
        T* Raw() { return &elements[0]; }

    private:
        IndexType FindNextFreeSlot( const IndexType prevFreeSlot )
        {
            IndexType startSlot = prevFreeSlot + 1;
            for ( ; startSlot < MAX_SIZE; ++startSlot )
            {
                if ( !slotsInUse[startSlot] )
                {
                    return startSlot;
                }
            }

            return NULL_VAL;
        }

        T elements[MAX_SIZE];
        std::bitset<MAX_SIZE> slotsInUse;
        IndexType elementCount;
        IndexType nextFreeSlot;
    };

    class UIScript
    {
    public:
        UIScript() = default;
        UIScript( sol::state *state, Script* s );
        UIScript( sol::state *state, const std::string& scriptName );

        sol::environment env;
        Script* script = nullptr;
    };

    struct LayoutInfo
    {
        std::string name;
        UIElementHandle rootElementHandle;
        uint16_t elementCount;
        UIScript* uiscript;
    };

    struct UIElementFunctions
    {
        UIScript* uiScript;
        sol::function update;
        sol::function mouseEnter;
        sol::function mouseLeave;
        sol::function mouseButtonDown;
        sol::function mouseButtonUp;
    };

} // namespace PG::UI