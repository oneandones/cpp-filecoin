/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/impl/state_tree_impl.hpp"

#include "vm/actor/builtin/init/init_actor.hpp"

namespace fc::vm::state {
  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store)
      : store_{store}, by_id{store} {}

  StateTreeImpl::StateTreeImpl(const std::shared_ptr<IpfsDatastore> &store,
                               const CID &root)
      : store_{store}, by_id{root, store} {}

  outcome::result<void> StateTreeImpl::set(const Address &address,
                                           const Actor &actor) {
    OUTCOME_TRY(address_id, lookupId(address));
    return by_id.set(address_id, actor);
  }

  outcome::result<Actor> StateTreeImpl::get(const Address &address) {
    OUTCOME_TRY(address_id, lookupId(address));
    return by_id.get(address_id);
  }

  outcome::result<Address> StateTreeImpl::lookupId(const Address &address) {
    if (address.getProtocol() == primitives::address::Protocol::ID) {
      return address;
    }
    OUTCOME_TRY(init_actor, get(actor::kInitAddress));
    OUTCOME_TRY(
        init_actor_state,
        store_->getCbor<actor::builtin::init::InitActorState>(init_actor.head));
    init_actor_state.address_map.load(store_);
    OUTCOME_TRY(id, init_actor_state.address_map.get(address));
    return Address::makeFromId(id);
  }

  outcome::result<Address> StateTreeImpl::registerNewAddress(
      const Address &address) {
    OUTCOME_TRY(init_actor, get(actor::kInitAddress));
    OUTCOME_TRY(
        init_actor_state,
        store_->getCbor<actor::builtin::init::InitActorState>(init_actor.head));
    init_actor_state.address_map.load(store_);
    OUTCOME_TRY(address_id, init_actor_state.addActor(address));
    OUTCOME_TRY(init_actor_state.address_map.flush());
    OUTCOME_TRYA(init_actor.head, store_->setCbor(init_actor_state));
    OUTCOME_TRY(set(actor::kInitAddress, init_actor));
    return std::move(address_id);
  }

  outcome::result<CID> StateTreeImpl::flush() {
    OUTCOME_TRY(by_id.flush());
    return by_id.hamt.cid();
  }

  outcome::result<void> StateTreeImpl::revert(const CID &root) {
    by_id = {root, store_};
    return outcome::success();
  }

  std::shared_ptr<IpfsDatastore> StateTreeImpl::getStore() {
    return store_;
  }
}  // namespace fc::vm::state
