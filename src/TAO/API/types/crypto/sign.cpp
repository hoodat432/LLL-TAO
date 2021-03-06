/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generates a signature for the data based on the private key for the keyname/user/pass/pin combination. */
        json::json Crypto::Sign(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Authenticate the users credentials */
            if(!users->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw APIException(-88, "Missing name.");
            
            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();
            
            /* Check the caller included the data */
            if(params.find("data") == params.end() || params["data"].get<std::string>().empty())
                throw APIException(-18, "Missing data.");

            /* Decode the data into a vector of bytes */
            std::string strData = params["data"].get<std::string>();
            std::vector<uint8_t> vchData(strData.begin(), strData.end());
            
            /* Get the private key. */
            uint512_t hashSecret = session.GetAccount()->Generate(strName, 0, strPIN);

            /* Buffer to receive the signature */
            std::vector<uint8_t> vchSig;

            /* Buffer to receive the public key */
            std::vector<uint8_t> vchPubKey;

            /* Get the scheme */
            uint8_t nScheme = get_scheme(params);

            /* Generate the signature */
            if(!session.GetAccount()->Sign(nScheme, vchData, hashSecret, vchPubKey, vchSig))
                throw APIException(-273, "Failed to generate signature");

            ret["publickey"] = encoding::EncodeBase58(vchPubKey);

            /* convert the scheme type to a string */
            switch(nScheme)
            {
                case TAO::Ledger::SIGNATURE::FALCON:
                    ret["scheme"] = "FALCON";
                    break;
                case TAO::Ledger::SIGNATURE::BRAINPOOL:
                    ret["scheme"] = "BRAINPOOL";
                    break;
                default:
                    ret["scheme"] = "";

            }

            ret["signature"] = encoding::EncodeBase64(&vchSig[0], vchSig.size());

            return ret;
        }
    }
}
