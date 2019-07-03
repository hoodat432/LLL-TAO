/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_OBJECTS_TOKEN_H
#define NEXUS_TAO_REGISTER_OBJECTS_TOKEN_H

#include <stdio.h>
#include <cstdint>

#include <Util/templates/serialize.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Token
         *
         *  Holds the state of parameters of a given token.
         *
         **/
        class Token
        {
        public:

            /** The version of this object register. **/
            uint32_t nVersion;


            /** The identifier of the account token. **/
            uint32_t nIdentifier;


            /** The maximum supply of said token. **/
            uint64_t nMaxSupply;


            /** The current token supply. */
            uint64_t nBalance;


            /** The significant figures of said token. **/
            uint8_t  nCoinDigits;


            /** Serialization **/
            IMPLEMENT_SERIALIZE
            (
                READWRITE(nVersion);
                READWRITE(nIdentifier);
                READWRITE(nMaxSupply);
                READWRITE(nBalance);
                READWRITE(nCoinDigits);
            )


            /** Default Constructor. **/
            Token()
            : nVersion(1)
            , nIdentifier(0)
            , nMaxSupply(0)
            , nBalance(0)
            , nCoinDigits(0)
            {

            }


            /** Constructor
             *
             *  Builds Token with specified parameters. Default digits is 6 significant figures.
             *
             *  @param[in] nIdentifierIn The identifier in
             *  @param[in] nMaxSupplyIn The max supply of token
             *
             **/
            Token(uint32_t nIdentifierIn, uint64_t nMaxSupplyIn)
            : nVersion(1)
            , nIdentifier(nIdentifierIn)
            , nMaxSupply(nMaxSupplyIn)
            , nBalance(nMaxSupply)
            , nCoinDigits(6)
            {

            }


            /** Consturctor
             *
             *  Builds Token with specified parameters.
             *
             *  @param[in] nIdentifierIn The identifier in
             *  @param[in] nMaxSupplyIn The max supply of token
             *  @param[in] nCoinDigitsIn The significant figures of token
             *
             **/
            Token(uint32_t nIdentifierIn, uint64_t nMaxSupplyIn, uint8_t nCoinDigitsIn)
            : nVersion(1)
            , nIdentifier(nIdentifierIn)
            , nMaxSupply(nMaxSupplyIn)
            , nBalance(nMaxSupplyIn)
            , nCoinDigits(nCoinDigitsIn)
            {

            }


            /** SetNull
             *
             *  Set this object to null state.
             *
             **/
            void SetNull()
            {
                nVersion       = 0;
                nIdentifier    = 0;
                nMaxSupply     = 0;
                nCoinDigits    = 0;
            }


            /** IsNull
             *
             *  Checks if object is in null state
             *
             **/
            bool IsNull() const
            {
                return (nVersion == 0 && nIdentifier == 0 && nMaxSupply == 0 && nCoinDigits == 0);
            }


            /** print
             *
             *  Output the state of this object.
             *
             **/
            void print() const
            {
                debug::log(0, "Token(version=", nVersion,
                    ", id=", nIdentifier,
                    ", maxsupply=", nMaxSupply,
                    ", digits=)", nCoinDigits);
            }
        };
    }
}

#endif