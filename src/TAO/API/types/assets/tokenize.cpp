/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Tokenize an asset into fungible tokens that represent ownership. */
        json::json Assets::Tokenize(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Authenticate the users credentials */
            if(!users->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Get the register address. */
            TAO::Register::Address hashToken;

            /* Check for data parameter. */
            if(params.find("token_name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashToken = Names::ResolveAddress(params, params["token_name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("token") != params.end() && IsRegisterAddress(params["token"]))
                hashToken.SetBase58(params["token"]);

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-37, "Missing token name / address");

            /* Get the register address. */
            TAO::Register::Address hashRegister ;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"]);

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-38, "Missing asset name / address");

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Submit the payload object.
               NOTE we pass true for the fForceTransfer parameter so that the transfer is made immediately to the
               token without requiring a Claim */
            tx[0] << (uint8_t)TAO::Operation::OP::TRANSFER << hashRegister << hashToken << uint8_t(TAO::Operation::TRANSFER::FORCE);

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-30, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();
            ret["address"] = hashRegister.ToString();

            return ret;
        }
    }
}
