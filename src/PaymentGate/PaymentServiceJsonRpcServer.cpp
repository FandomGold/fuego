// Copyright (c) 2017-2022 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "PaymentServiceJsonRpcServer.h"

#include <functional>

#include "PaymentServiceJsonRpcMessages.h"
#include "WalletService.h"
#include "Common/CommandLine.h"
#include "Common/StringTools.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/Account.h"
#include "crypto/hash.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "WalletLegacy/WalletHelper.h"
#include "Common/Base58.h"
#include "Common/CommandLine.h"
#include "Common/SignalHandler.h"
#include "Common/StringTools.h"
#include "Common/PathTools.h"
#include "Common/Util.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "Serialization/JsonInputValueSerializer.h"
#include "Serialization/JsonOutputStreamSerializer.h"

namespace PaymentService {

PaymentServiceJsonRpcServer::PaymentServiceJsonRpcServer(System::Dispatcher& sys, System::Event& stopEvent, WalletService& service, Logging::ILogger& loggerGroup) 
  : JsonRpcServer(sys, stopEvent, loggerGroup)
  , service(service)
  , logger(loggerGroup, "PaymentServiceJsonRpcServer")
{
  handlers.emplace("save", jsonHandler<Save::Request, Save::Response>(std::bind(&PaymentServiceJsonRpcServer::handleSave, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("createIntegrated", jsonHandler<CreateIntegrated::Request, CreateIntegrated::Response>(std::bind(&PaymentServiceJsonRpcServer::handleCreateIntegrated, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("splitIntegrated", jsonHandler<SplitIntegrated::Request, SplitIntegrated::Response>(std::bind(&PaymentServiceJsonRpcServer::handleSplitIntegrated, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("reset", jsonHandler<Reset::Request, Reset::Response>(std::bind(&PaymentServiceJsonRpcServer::handleReset, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("exportWallet", jsonHandler<ExportWallet::Request, ExportWallet::Response>(std::bind(&PaymentServiceJsonRpcServer::handleExportWallet, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("exportWalletKeys", jsonHandler<ExportWalletKeys::Request, ExportWalletKeys::Response>(std::bind(&PaymentServiceJsonRpcServer::handleExportWalletKeys, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("createAddress", jsonHandler<CreateAddress::Request, CreateAddress::Response>(std::bind(&PaymentServiceJsonRpcServer::handleCreateAddress, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("createAddressList", jsonHandler<CreateAddressList::Request, CreateAddressList::Response>(std::bind(&PaymentServiceJsonRpcServer::handleCreateAddressList, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("deleteAddress", jsonHandler<DeleteAddress::Request, DeleteAddress::Response>(std::bind(&PaymentServiceJsonRpcServer::handleDeleteAddress, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getSpendKeys", jsonHandler<GetSpendKeys::Request, GetSpendKeys::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetSpendKeys, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getBalance", jsonHandler<GetBalance::Request, GetBalance::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetBalance, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getBlockHashes", jsonHandler<GetBlockHashes::Request, GetBlockHashes::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetBlockHashes, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getTransactionHashes", jsonHandler<GetTransactionHashes::Request, GetTransactionHashes::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetTransactionHashes, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getTransactions", jsonHandler<GetTransactions::Request, GetTransactions::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetTransactions, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getUnconfirmedTransactionHashes", jsonHandler<GetUnconfirmedTransactionHashes::Request, GetUnconfirmedTransactionHashes::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetUnconfirmedTransactionHashes, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getTransaction", jsonHandler<GetTransaction::Request, GetTransaction::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetTransaction, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("sendTransaction", jsonHandler<SendTransaction::Request, SendTransaction::Response>(std::bind(&PaymentServiceJsonRpcServer::handleSendTransaction, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("createDelayedTransaction", jsonHandler<CreateDelayedTransaction::Request, CreateDelayedTransaction::Response>(std::bind(&PaymentServiceJsonRpcServer::handleCreateDelayedTransaction, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getDelayedTransactionHashes", jsonHandler<GetDelayedTransactionHashes::Request, GetDelayedTransactionHashes::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetDelayedTransactionHashes, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("deleteDelayedTransaction", jsonHandler<DeleteDelayedTransaction::Request, DeleteDelayedTransaction::Response>(std::bind(&PaymentServiceJsonRpcServer::handleDeleteDelayedTransaction, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("sendDelayedTransaction", jsonHandler<SendDelayedTransaction::Request, SendDelayedTransaction::Response>(std::bind(&PaymentServiceJsonRpcServer::handleSendDelayedTransaction, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getViewKey", jsonHandler<GetViewKey::Request, GetViewKey::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetViewKey, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getStatus", jsonHandler<GetStatus::Request, GetStatus::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetStatus, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getAddresses", jsonHandler<GetAddresses::Request, GetAddresses::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetAddresses, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("createDeposit", jsonHandler<CreateDeposit::Request, CreateDeposit::Response>(std::bind(&PaymentServiceJsonRpcServer::handleCreateDeposit, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("sendDeposit", jsonHandler<SendDeposit::Request, SendDeposit::Response>(std::bind(&PaymentServiceJsonRpcServer::handleSendDeposit, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("withdrawDeposit", jsonHandler<WithdrawDeposit::Request, WithdrawDeposit::Response>(std::bind(&PaymentServiceJsonRpcServer::handleWithdrawDeposit, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getMessagesFromExtra", jsonHandler<GetMessagesFromExtra::Request, GetMessagesFromExtra::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetMessagesFromExtra, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("getDeposit", jsonHandler<GetDeposit::Request, GetDeposit::Response>(std::bind(&PaymentServiceJsonRpcServer::handleGetDeposit, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("estimateFusion", jsonHandler<EstimateFusion::Request, EstimateFusion::Response>(std::bind(&PaymentServiceJsonRpcServer::handleEstimateFusion, this, std::placeholders::_1, std::placeholders::_2)));
  handlers.emplace("sendFusionTransaction", jsonHandler<SendFusionTransaction::Request, SendFusionTransaction::Response>(std::bind(&PaymentServiceJsonRpcServer::handleSendFusionTransaction, this, std::placeholders::_1, std::placeholders::_2)));
}

void PaymentServiceJsonRpcServer::processJsonRpcRequest(const Common::JsonValue& req, Common::JsonValue& resp) {
  try {
    prepareJsonResponse(req, resp);

    if (!req.contains("method")) {
      logger(Logging::WARNING) << "Field \"method\" is not found in json request: " << req;
      makeGenericErrorReponse(resp, "Invalid Request", -3600);
      return;
    }

    if (!req("method").isString()) {
      logger(Logging::WARNING) << "Field \"method\" is not a string type: " << req;
      makeGenericErrorReponse(resp, "Invalid Request", -3600);
      return;
    }

    std::string method = req("method").getString();

    auto it = handlers.find(method);
    if (it == handlers.end()) {
      logger(Logging::WARNING) << "Requested method not found: " << method;
      makeMethodNotFoundResponse(resp);
      return;
    }

    logger(Logging::DEBUGGING) << method << " request came";

    Common::JsonValue params(Common::JsonValue::OBJECT);
    if (req.contains("params")) {
      params = req("params");
    }

    it->second(params, resp);
  } catch (std::exception& e) {
    logger(Logging::WARNING) << "Error occurred while processing JsonRpc request: " << e.what();
    makeGenericErrorReponse(resp, e.what());
  }
}

std::error_code PaymentServiceJsonRpcServer::handleReset(const Reset::Request& request, Reset::Response& response) {
  if (request.viewSecretKey.empty()) {
    if (request.scanHeight != std::numeric_limits<uint32_t>::max()) {
      return service.resetWallet(request.scanHeight);
    } else {
      return service.resetWallet();
    }
  } else {
    if (request.scanHeight != std::numeric_limits<uint32_t>::max()) {
      return service.replaceWithNewWallet(request.viewSecretKey);
    } else {
      return service.replaceWithNewWallet(request.viewSecretKey);
    }
  }
}

std::error_code PaymentServiceJsonRpcServer::handleCreateAddress(const CreateAddress::Request& request, CreateAddress::Response& response) {
  if (request.spendSecretKey.empty() && request.spendPublicKey.empty()) {
    return service.createAddress(response.address);
  } else if (!request.spendSecretKey.empty()) {
    return service.createAddress(request.spendSecretKey, response.address);
  } else {
    return service.createTrackingAddress(request.spendPublicKey, response.address);
  }
}

std::error_code PaymentServiceJsonRpcServer::handleExportWallet(const ExportWallet::Request &request, ExportWallet::Response &response)
{
  return service.exportWallet(request.exportFilename);
}

std::error_code PaymentServiceJsonRpcServer::handleExportWalletKeys(const ExportWalletKeys::Request &request, ExportWalletKeys::Response &response)
{
  return service.exportWalletKeys(request.exportFilename);
}

std::error_code PaymentServiceJsonRpcServer::handleCreateAddressList(const CreateAddressList::Request& request, CreateAddressList::Response& response) {
  return service.createAddressList(request.spendSecretKeys, request.reset, response.addresses);
}

std::error_code PaymentServiceJsonRpcServer::handleSave(const Save::Request& /*request*/, Save::Response& /*response*/) 
{
  return service.saveWalletNoThrow();
}


std::error_code PaymentServiceJsonRpcServer::handleCreateIntegrated(const CreateIntegrated::Request& request, CreateIntegrated::Response& response) 
{
  return service.createIntegratedAddress(request, response.integrated_address);
}

std::error_code PaymentServiceJsonRpcServer::handleSplitIntegrated(const SplitIntegrated::Request& request, SplitIntegrated::Response& response) 
{
  return service.splitIntegratedAddress(request, response.address, response.payment_id);
}
std::error_code PaymentServiceJsonRpcServer::handleDeleteAddress(const DeleteAddress::Request& request, DeleteAddress::Response& response) {
  return service.deleteAddress(request.address);
}

std::error_code PaymentServiceJsonRpcServer::handleGetSpendKeys(const GetSpendKeys::Request& request, GetSpendKeys::Response& response) {
  return service.getSpendkeys(request.address, response.spendPublicKey, response.spendSecretKey);
}

std::error_code PaymentServiceJsonRpcServer::handleGetBalance(const GetBalance::Request& request, GetBalance::Response& response) {
  if (!request.address.empty()) {
    return service.getBalance(request.address, response.availableBalance, response.lockedAmount, response.lockedDepositBalance, response.unlockedDepositBalance);
  } else {
    return service.getBalance(response.availableBalance, response.lockedAmount, response.lockedDepositBalance, response.unlockedDepositBalance);
  }
}

std::error_code PaymentServiceJsonRpcServer::handleGetBlockHashes(const GetBlockHashes::Request& request, GetBlockHashes::Response& response) {
  return service.getBlockHashes(request.firstBlockIndex, request.blockCount, response.blockHashes);
}

std::error_code PaymentServiceJsonRpcServer::handleGetTransactionHashes(const GetTransactionHashes::Request& request, GetTransactionHashes::Response& response) {
  if (!request.blockHash.empty()) {
    return service.getTransactionHashes(request.addresses, request.blockHash, request.blockCount, request.paymentId, response.items);
  } else {
    return service.getTransactionHashes(request.addresses, request.firstBlockIndex, request.blockCount, request.paymentId, response.items);
  }
}

std::error_code PaymentServiceJsonRpcServer::handleGetTransactions(const GetTransactions::Request& request, GetTransactions::Response& response) {
  if (!request.blockHash.empty()) {
    return service.getTransactions(request.addresses, request.blockHash, request.blockCount, request.paymentId, response.items);
  } else {
    return service.getTransactions(request.addresses, request.firstBlockIndex, request.blockCount, request.paymentId, response.items);
  }
}

std::error_code PaymentServiceJsonRpcServer::handleGetUnconfirmedTransactionHashes(const GetUnconfirmedTransactionHashes::Request& request, GetUnconfirmedTransactionHashes::Response& response) {
  return service.getUnconfirmedTransactionHashes(request.addresses, response.transactionHashes);
}

std::error_code PaymentServiceJsonRpcServer::handleGetTransaction(const GetTransaction::Request& request, GetTransaction::Response& response) {
  return service.getTransaction(request.transactionHash, response.transaction);
}

std::error_code PaymentServiceJsonRpcServer::handleSendTransaction(const SendTransaction::Request& request, SendTransaction::Response& response) {
  return service.sendTransaction(request, response.transactionHash, response.transactionSecretKey);
}

std::error_code PaymentServiceJsonRpcServer::handleCreateDelayedTransaction(const CreateDelayedTransaction::Request& request, CreateDelayedTransaction::Response& response) {
  return service.createDelayedTransaction(request, response.transactionHash);
}

std::error_code PaymentServiceJsonRpcServer::handleGetDelayedTransactionHashes(const GetDelayedTransactionHashes::Request& request, GetDelayedTransactionHashes::Response& response) {
  return service.getDelayedTransactionHashes(response.transactionHashes);
}

std::error_code PaymentServiceJsonRpcServer::handleDeleteDelayedTransaction(const DeleteDelayedTransaction::Request& request, DeleteDelayedTransaction::Response& response) {
  return service.deleteDelayedTransaction(request.transactionHash);
}

std::error_code PaymentServiceJsonRpcServer::handleSendDelayedTransaction(const SendDelayedTransaction::Request& request, SendDelayedTransaction::Response& response) {
  return service.sendDelayedTransaction(request.transactionHash);
}

std::error_code PaymentServiceJsonRpcServer::handleGetViewKey(const GetViewKey::Request& request, GetViewKey::Response& response) {
  return service.getViewKey(response.viewSecretKey);
}

std::error_code PaymentServiceJsonRpcServer::handleGetStatus(const GetStatus::Request& request, GetStatus::Response& response) {
  return service.getStatus(response.blockCount, response.knownBlockCount, response.lastBlockHash, response.peerCount, response.depositCount, response.transactionCount, response.addressCount);
}

std::error_code PaymentServiceJsonRpcServer::handleCreateDeposit(const CreateDeposit::Request& request, CreateDeposit::Response& response) {
  return service.createDeposit(request.amount, request.term, request.sourceAddress, response.transactionHash);
}

std::error_code PaymentServiceJsonRpcServer::handleWithdrawDeposit(const WithdrawDeposit::Request &request, WithdrawDeposit::Response &response)
{
  return service.withdrawDeposit(request.depositId, response.transactionHash);
}

std::error_code PaymentServiceJsonRpcServer::handleSendDeposit(const SendDeposit::Request& request, SendDeposit::Response& response) {
  return service.sendDeposit(request.amount, request.term, request.sourceAddress, request.destinationAddress, response.transactionHash);
}

std::error_code PaymentServiceJsonRpcServer::handleGetDeposit(const GetDeposit::Request& request, GetDeposit::Response& response) {
  return service.getDeposit(request.depositId, response.amount, response.term, response.interest, response.creatingTransactionHash, response.spendingTransactionHash, response.locked, response.height, response.unlockHeight, response.address);
}

std::error_code PaymentServiceJsonRpcServer::handleGetAddresses(const GetAddresses::Request& request, GetAddresses::Response& response) {
  return service.getAddresses(response.addresses);
}

std::error_code PaymentServiceJsonRpcServer::handleGetMessagesFromExtra(const GetMessagesFromExtra::Request& request, GetMessagesFromExtra::Response& response) {
  return service.getMessagesFromExtra(request.extra, response.messages);
}

std::error_code PaymentServiceJsonRpcServer::handleEstimateFusion(const EstimateFusion::Request& request, EstimateFusion::Response& response) {
  return service.estimateFusion(request.threshold, request.addresses, response.fusionReadyCount, response.totalOutputCount);
}


std::error_code PaymentServiceJsonRpcServer::handleSendFusionTransaction(const SendFusionTransaction::Request& request, SendFusionTransaction::Response& response) {
  return service.sendFusionTransaction(request.threshold, request.anonymity, request.addresses, request.destinationAddress, response.transactionHash);
}

}
