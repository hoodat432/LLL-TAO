/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/types/register.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <Util/include/memory.h>

namespace LLD
{

    /* Register transaction to track current open transaction. */
    thread_local std::unique_ptr<RegisterTransaction> RegisterDB::pMemory;


    /* Miner transaction to track current states for miner verification. */
    thread_local std::unique_ptr<RegisterTransaction> RegisterDB::pMiner;


    /* The Database Constructor. To determine file location and the Bytes per Record. */
    RegisterDB::RegisterDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_REGISTER")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)

    , MEMORY_MUTEX()
    , pCommit(new RegisterTransaction())
    {
    }


    /* Default Destructor */
    RegisterDB::~RegisterDB()
    {
        /* Cleanup commited states. */
        if(pCommit)
            delete pCommit;
    }


    /*  Writes a state register to the register database.
     *  If MEMPOOL flag is set, this will write state into a temporary
     *  memory to handle register state sequencing before blocks commit. */
    bool RegisterDB::WriteState(const uint256_t& hashRegister, const TAO::Register::State& state, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for memory mode. */
            if(pMemory)
            {
                /* Check erase queue. */
                pMemory->mapErase.erase(hashRegister);
                pMemory->mapStates[hashRegister] = state;

                return true;
            }

            {
                LOCK(MEMORY_MUTEX);

                /* Otherwise commit like normal. */
                pCommit->mapStates[hashRegister] = state;
            }

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Check for memory mode. */
            if(pMiner)
            {
                pMiner->mapErase.erase(hashRegister);
                pMiner->mapStates[hashRegister] = state;
            }


            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Remove the memory state if writing the disk state. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Check for most recent memory state, and remove if writing it. */
                const TAO::Register::State& stateCheck = pCommit->mapStates[hashRegister];
                if(stateCheck == state || nFlags == TAO::Ledger::FLAGS::ERASE)
                {
                    /* Erase if transaction. */
                    if(pMemory)
                    {
                        /* Erase from ACID transaction. */
                        pMemory->mapStates.erase(hashRegister);
                        pMemory->mapErase.insert(std::make_pair(hashRegister, state));
                    }
                    else
                        pCommit->mapStates.erase(hashRegister);
                }
            }

            /* Quit when erasing. */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        /* Add sequential read keys for known address types. */
        std::string strType = "NONE";
        switch(hashRegister.GetType())
        {
            case TAO::Register::Address::ACCOUNT:
                strType = "account";
                break;

            case TAO::Register::Address::APPEND:
                strType = "append";
                break;

            case TAO::Register::Address::CRYPTO:
                strType = "crypto";
                break;

            case TAO::Register::Address::NAME:
                strType = "name";
                break;

            case TAO::Register::Address::NAMESPACE:
                strType = "namespace";
                break;

            case TAO::Register::Address::OBJECT:
                strType = "object";
                break;

            case TAO::Register::Address::RAW:
                strType = "raw";
                break;

            case TAO::Register::Address::READONLY:
                strType = "readonly";
                break;

            case TAO::Register::Address::TOKEN:
                strType = "token";
                break;

            case TAO::Register::Address::TRUST:
                strType = "trust";
                break;

            default :
                strType = "NONE";
        }

        /* Write the state to the register database */
        return Write(std::make_pair(std::string("state"), hashRegister), state, strType);
    }


    /* Read a state register from the register database. */
    bool RegisterDB::ReadState(const uint256_t& hashRegister, TAO::Register::State& state, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for a memory transaction first */
            if(pMemory && pMemory->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMemory->mapStates[hashRegister];

                return true;
            }

            {
                LOCK(MEMORY_MUTEX);

                /* Check for state in memory map. */
                if(pCommit->mapStates.count(hashRegister))
                {
                    /* Get the state from commited memory. */
                    state = pCommit->mapStates[hashRegister];

                    return true;
                }
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Check for a memory transaction first */
            if(pMiner && pMiner->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMiner->mapStates[hashRegister];

                return true;
            }
        }

        return Read(std::make_pair(std::string("state"), hashRegister), state);
    }


    /* Erase a state register from the register database. */
    bool RegisterDB::EraseState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Check for memory transaction. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for available states. */
            if(pMemory)
            {
                pMemory->mapStates.erase(hashRegister);

                /* Erase state from mempool. */
                {
                    LOCK(MEMORY_MUTEX);
                    if(pCommit->mapStates.count(hashRegister))
                        pMemory->mapErase.insert(std::make_pair(hashRegister, pCommit->mapStates[hashRegister]));
                }

                return true;
            }

            /* Erase state from mempool. */
            {
                LOCK(MEMORY_MUTEX);
                pCommit->mapStates.erase(hashRegister);
            }

            return true;
        }
        else if(nFlags == TAO::Ledger::FLAGS::BLOCK || nFlags == TAO::Ledger::FLAGS::ERASE)
        {
            LOCK(MEMORY_MUTEX);

            /* Erase from memory if on block. */
            if(pCommit->mapStates.count(hashRegister))
            {
                /* Check for transction. */
                if(pMemory)
                {
                    /* Erase from ACID transaction. */
                    pMemory->mapStates.erase(hashRegister);

                    /* Erase state from mempool. */
                    if(pCommit->mapStates.count(hashRegister))
                        pMemory->mapErase.insert(std::make_pair(hashRegister, pCommit->mapStates[hashRegister]));
                }
                else
                    pCommit->mapStates.erase(hashRegister);
            }

            /* Break on erase.  */
            if(nFlags == TAO::Ledger::FLAGS::ERASE)
                return true;
        }

        return Erase(std::make_pair(std::string("state"), hashRegister));
    }


    /* Index a genesis to a register address. */
    bool RegisterDB::IndexTrust(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Index(std::make_pair(std::string("genesis"), hashGenesis), std::make_pair(std::string("state"), hashRegister));
    }


    /* Check that a genesis doesn't already exist. */
    bool RegisterDB::HasTrust(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Write a genesis to a register address. */
    bool RegisterDB::WriteTrust(const uint256_t& hashGenesis, const TAO::Register::State& state)
    {
        /* Get trust account address for contract caller */
        uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        {
            LOCK(MEMORY_MUTEX);

            /* Remove the memory state if writing the disk state. */
            if(pCommit->mapStates.count(hashRegister))
                pCommit->mapStates.erase(hashRegister);
        }

        /* We want to write the state like normal, but ensure we wipe memory states. */
        return WriteState(hashRegister, state);
    }


    /* Index a genesis to a register address. */
    bool RegisterDB::ReadTrust(const uint256_t& hashGenesis, TAO::Register::State& state)
    {
        /* Get trust account address for contract caller */
        uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check for a memory transaction first */
            if(pMemory && pMemory->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMemory->mapStates[hashRegister];

                return true;
            }

            /* Check for state in memory map. */
            {
                LOCK(MEMORY_MUTEX);

                if(pCommit->mapStates.count(hashRegister))
                {
                    /* Get the state from commited memory. */
                    state = pCommit->mapStates[hashRegister];

                    return true;
                }
            }
        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Check for a memory transaction first */
            if(pMiner && pMiner->mapStates.count(hashRegister))
            {
                /* Get the state from temporary transaction. */
                state = pMiner->mapStates[hashRegister];

                return true;
            }
        }

        return Read(std::make_pair(std::string("genesis"), hashGenesis), state);
    }


    /* Erase a genesis from a register address. */
    bool RegisterDB::EraseTrust(const uint256_t& hashGenesis)
    {
        /* Get trust account address for contract caller */
        uint256_t hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        return Erase(std::make_pair(std::string("genesis"), hashGenesis));
    }


    /* Determines if a state exists in the register database. */
    bool RegisterDB::HasState(const uint256_t& hashRegister, const uint8_t nFlags)
    {
        /* Memory mode for pre-database commits. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            /* Check internal memory state. */
            if(pMemory && pMemory->mapStates.count(hashRegister))
                return true;

            /* Check for state in memory map. */
            {
                LOCK(MEMORY_MUTEX);

                if(pCommit->mapStates.count(hashRegister))
                    return true;
            }

        }
        else if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Check internal memory state. */
            if(pMiner && pMiner->mapStates.count(hashRegister))
                return true;
        }

        return Exists(std::make_pair(std::string("state"), hashRegister));
    }

    /* Begin a memory transaction following ACID properties. */
    void RegisterDB::MemoryBegin(const uint8_t nFlags)
    {
        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            pMiner = std::unique_ptr<RegisterTransaction>(new RegisterTransaction());

            return;
        }

        /* Set the pre-commit memory mode. */
        pMemory = std::unique_ptr<RegisterTransaction>(new RegisterTransaction());
    }


    /* Abort a memory transaction following ACID properties. */
    void RegisterDB::MemoryRelease(const uint8_t nFlags)
    {
        /* Check for miner. */
        if(nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Set the pre-commit memory mode. */
            pMiner = nullptr;

            return;
        }

        /* Set the pre-commit memory mode. */
        pMemory = nullptr;
    }


    /* Commit a memory transaction following ACID properties. */
    void RegisterDB::MemoryCommit()
    {
        /* Abort the current memory mode. */
        if(pMemory)
        {
            LOCK(MEMORY_MUTEX);

            /* Loop through all new states and apply to commit data. */
            for(const auto& state : pMemory->mapStates)
                pCommit->mapStates[state.first] = state.second;

            /* Loop through values to erase. */
            for(const auto& erase : pMemory->mapErase)
            {
                /* Check current commited state. */
                if(pCommit->mapStates.count(erase.first))
                {
                    /* Get the current register's state. */
                    const TAO::Register::State& state = pCommit->mapStates[erase.first];
                    if(erase.second != state)
                    {
                        debug::log(0, FUNCTION, ANSI_COLOR_BRIGHT_YELLOW, "CONFLICTED STATE ", ANSI_COLOR_RESET, erase.first.SubString());

                        TAO::Register::Object object1 = TAO::Register::Object(state);
                        object1.Parse();

                        TAO::Register::Object object2 = TAO::Register::Object(erase.second);
                        object2.Parse();

                        debug::log(0, FUNCTION, "Balance (COMM): ", object1.get<uint64_t>("balance"));
                        debug::log(0, FUNCTION, "Balance (ACID): ", object2.get<uint64_t>("balance"));

                        continue;
                    }

                    pCommit->mapStates.erase(erase.first);
                    debug::log(0, "ERASING entry ", erase.first.SubString());
                }
            }

            /* Free the memory. */
            pMemory = nullptr;
        }
    }
}
