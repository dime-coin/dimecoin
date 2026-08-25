# Dimecoin Core Development Roadmap

This roadmap describes the general development direction established by the Dimecoin Core maintainers. It is intended to communicate priorities and sequencing without assigning speculative release dates or guaranteeing that a particular proposal will be adopted.

Individual consensus changes and major features will still require their own design proposals, technical review, test plans, and deployment decisions. An open issue or pull request should not be interpreted as an accepted roadmap commitment unless it has been reviewed and assigned to a roadmap milestone by the maintainers.

## Development path

**Stability and overdue maintenance → major consensus updates → emergency maintenance as required → exchange readiness → innovation**

Emergency security, consensus, network, or wallet fixes may interrupt any phase. Protecting the live network and user funds takes priority over the planned sequence.

## 1. Stability and overdue maintenance

**Current milestone: Dimecoin Core 2.4.0.0 — in progress**

The immediate priority is to deliver a stable, network compatible release that addresses accumulated technical debt without introducing new consensus rules or requiring a coordinated network activation.

### Primary work

- Improve initial synchronization, stalled download recovery, peer handling, checkpoint behavior, and chain state reliability.
- Correct wallet balance, maturity, staking, coin selection, fee, database, import/export, and recovery behavior.
- Harden masternode, governance, signed message, and payment related code while preserving the rules enforced by the current network.
- Add safer handling for malformed input, bounded resource use, race conditions, locking, cache state, and component shutdown.
- Refresh dependencies and restore reliable builds on supported Linux and Windows environments.
- Update bootstrap peers, DNS seed data, CI, functional testing, build documentation, release metadata, and remaining user facing Dimecoin terminology.
- Expand unit and functional coverage for the wallet, networking, validation, masternodes, message signing, and resource limits.

### Release boundary

Version 2.4.0.0 is not the place for new monetary policy, new activation heights, dormant feature activation, speculative protocol additions, or unrelated cosmetic refactoring. Those changes should be reviewed independently after the stabilization release.

### Completion criteria

- Supported native and dependency based builds complete successfully.
- Unit and functional test suites pass on supported platforms.
- Fresh mainnet and testnet synchronization is verified from genesis.
- Upgrade, wallet backup, release note, checksum, and binary signing procedures are complete.
- Release receives independent review and network testing before publication.

## 2. Major consensus and protocol updates

Consensus work will follow 2.4.0.0 and will be developed in reviewable stages. Compatibility, deterministic behavior, and safe deployment are more important than attaching the work to a particular version number or date.

### 2.1 Document and lock down the existing consensus baseline

- Produce a clear specification of Dimecoin's hybrid PoW and PoS rules, activation history, block classification, reward selection, and payment enforcement.
- Add regression coverage for the post height-5,000,000 reward decay schedule and the existing miner/staker, masternode, and foundation payment allocation.
- Remove avoidable floating point monetary calculations using a reviewed integer formulation that preserves the current schedule exactly.
- Ensure consensus functions use the consensus parameters passed to them instead of silently depending on globally selected chain parameters.
- Test boundary heights, long range decay, alternate chain parameters, reorganization behavior, and both PoW and PoS produced blocks.
- Establish and periodically update appropriate chain-work and assume-valid references without weakening full-validation support.

This work must preserve the intended declining issuance schedule. It must not introduce an unapproved permanent reward floor or otherwise change Dimecoin's monetary policy under the description of a cleanup.

### 2.2 Strengthen hybrid PoW/PoS operation

- Review the interaction between the PoW and PoS difficulty paths and their effect on overall block cadence.
- Model periods where one block production arm becomes dominant, sparse, or temporarily inactive.
- Audit coinstake validation, block signatures, timestamp rules, reorganizations, checkpoints, and masternode/foundation payments across both block types.
- Develop simulations and adversarial tests before proposing changes to live difficulty or timing rules.
- Evaluate existing Quark mining and overall network security based on network conditions

### 2.3 Introduce deterministic masternodes

- Design an on chain deterministic masternode registry and deterministic payment selection.
- Define registration, update, revocation, collateral, operator key, and migration behavior.
- Preserve a replayable masternode state across reorganizations and historical validation.
- Provide a staged testnet migration and a clear compatibility plan for existing masternode operators.

Deterministic masternodes are a foundational dependency for later masternode backed services. Spork and governance modernization should be reviewed as a related but separate design track rather than being assumed as part of the registry change.

### 2.4 Modernize activation and validation infrastructure

- Review typed validation results so peer management code can distinguish permanent invalidity from contextual, transient, or network specific failures.
- Define a repeatable process for proposing, testing, signalling, monitoring, and activating future consensus changes.
- Evaluate SegWit through a dedicated Dimecoin proposal. The inherited deployment window expired without activation, and any new deployment must account for miners, stakers, coinstake transactions, block signatures, witness commitments, wallet behavior, and masternode/foundation payments.
- Consider newer Bitcoin protocol features through targeted, reviewable ports where they benefit Dimecoin. A wholesale rebase is not assumed to be a prerequisite for every modernization effort.
- Keep Taproot and other later script upgrades in research until their dependencies, use cases, and deployment path are established.
- Deploy and enforce MN pays for the POW side
- 
### Consensus change gate

No major consensus change should move to mainnet without:

- a written specification and compatibility analysis;
- deterministic unit, functional, boundary, and reorganization tests;
- economic and security review appropriate to the change;
- an extended public testnet deployment;
- independent code review and reproducible release candidates; and
- a documented activation, monitoring, and incident-response plan.

## 3. Emergency maintenance

Emergency maintenance is a standing workstream rather than a scheduled feature phase. Critical security vulnerabilities, consensus divergence, network stalls, wallet-funds risks, or release-blocking defects take priority whenever they appear.

Emergency releases should:

- remain narrowly scoped to the verified problem;
- preserve private and coordinated disclosure where public detail would place users or the network at risk;
- include a regression test whenever practical;
- avoid bundling unrelated features or refactoring; and
- be followed by a public explanation once disclosure is safe.

## 4. Exchange and service provider readiness

The goal of this phase is to make Dimecoin straightforward to evaluate, integrate, operate, and monitor. It does not promise a listing on any particular platform.

### 4.1 Canonical integration specification

Maintain one authoritative technical reference covering:

- network identifiers, genesis hashes, ports, address prefixes, message formats, and decimal precision;
- Quark Proof of Work, hybrid PoW/PoS block identification, expected confirmation behavior, and reorganization handling;
- transaction serialization, including Dimecoin-specific version-2 transaction comments;
- fee units, dust and relay policy, maturity rules, and wallet accounting;
- supported RPC, REST, ZMQ, transaction index, rescan, and block-notification behavior; and
- tested deposit, withdrawal, signing, fee-estimation, reorganization, backup, and recovery procedures.

### 4.2 Stable operational infrastructure

- Publish reproducible, signed Core binaries with checksums and upgrade instructions.
- Maintain reliable DNS seeds, fixed bootstrap nodes, explorers, ElectrumX-Dime servers, and public status information.
- Provide monitoring guidance for chain tip, peer count, wallet state, deposits, withdrawals, and network reorganizations.
- Maintain an exchange-focused functional test plan that service providers can reproduce before enabling deposits and withdrawals.

### 4.3 Wallet identity and ecosystem registration

- Obtain a dedicated SLIP-0044 coin type for DIME.
- Preserve recovery and compatibility for wallets created with legacy derivation paths.
- Migrate signed message identity only through a staged accept-before-emit compatibility plan so existing nodes, wallets, and masternodes are not isolated.
- Complete appropriate wallet, hardware-wallet, custody, market-data, and service-provider registrations using verified Dimecoin parameters.
- Stabilize and update the Dimecoin-specific Electrum and ElectrumX stack as supported integration infrastructure. Both stacks require updates and maintenance as well. Those roadmaps and guidelines can be reviewed in their individual repositories -- coming soon.

Commercial, organizational, and legal requirements imposed by an exchange are separate from the Core codebase and must be evaluated independently. Dimecoin is an open source project; the location of individual contributors does not by itself define a formal project entity or establish eligibility for a particular listing program.

### Readiness gate

This phase is complete when a competent third party operator can integrate DIME from maintained documentation and test fixtures without reverse engineering consensus behavior or relying on undocumented maintainer knowledge.

## 5. Innovation and ecosystem development

Innovation work begins from the stable consensus and integration foundations above. Items in this section are directions for staged research and development, not promises that every feature will ship.

### 5.1 Wallet and protocol capabilities

- Move Electrum-Dime and ElectrumX-Dime from beta infrastructure toward fully tested, documented releases.
- Evaluate BIP157/158 compact block filters as an optional light client path separate from ElectrumX.
- Validate PSBT, CSV, CLTV, multisignature, hardware wallet signing, and recovery workflows using Dimecoin's actual transaction format.
- Evaluate Taproot, improved transport privacy, and other targeted upstream protocol additions only after their dependencies and Dimecoin specific use cases are clear.

### 5.2 Non-custodial trading and interoperability

- Prototype DIME support using established UTXO integration frameworks such as DCRDEX or BasicSwap before considering a larger Dimecoin specific exchange platform.
- Test atomic swap redeem and refund paths across both PoW and PoS blocks, including confirmation policy, reorganizations, transaction indexing, five-decimal amounts, and Dimecoin-specific serialization.
- Submit integrations through the relevant upstream review processes and distinguish experimental community configurations from supported production integrations.
- Explore broader interoperability only after core wallet, signing, and consensus assumptions have been verified end to end.

### 5.3 Research potential Masternode backed services

The current research sequence is:

1. Research whether Dimecoin's masternode infrastructure can support useful services beyond its current network and payment functions.
2. Require each proposal to define the service being provided, why masternodes are appropriate for it, its security and trust assumptions, operator responsibilities, resource requirements, and sustainable incentives.
3. Evaluate deterministic behavior, failure handling, reorganizations, abuse resistance, privacy, governance boundaries, and compatibility with the existing network.
4. Begin with problem statements, simulations, and testnet prototypes rather than committing the project to named products or production deployments.
5. Do not alter consensus rules, masternode payments, collateral requirements, or monetary policy without a separate reviewed proposal and deployment plan.

### 5.4 Long term research

- Assess whether a future decentralized trading platform for UTXO and tokenized assets is justified after smaller integrations establish real requirements.
- Continue selective modernization against newer Bitcoin research and implementations without discarding Dimecoin specific hybrid consensus behavior.
- Evaluate additional masternode use cases only when they provide a defined network service, sustainable operator incentives, and enforceable security properties.

## How roadmap work is tracked

Each active roadmap item should have a focused GitHub issue or discussion that identifies:

- the problem being solved and whether it affects consensus;
- current behavior supported by code, tests, or reproducible evidence;
- scope, dependencies, risks, and backward-compatibility requirements;
- measurable acceptance criteria and a test plan;
- the responsible milestone and current status; and
- documentation and operator communication required for deployment.

Broad proposals should begin in GitHub Discussions. Implementable, bounded tasks should move into Issues. Consensus changes should not begin as large implementation pull requests without an agreed design and test strategy.

Contributor-friendly documentation, tests, build fixes, and isolated cleanups may be marked `good first issue`. Consensus, cryptography, wallet-security, and monetary-policy changes require experienced review and should not be labelled as introductory work merely because the code change appears small.

## Roadmap maintenance

This document should be reviewed after each stable Core release and whenever the maintainers accept, reject, or materially rescope a major proposal. Release milestones and linked issues are the authoritative source for work currently in progress; this roadmap provides the longer-term direction.
