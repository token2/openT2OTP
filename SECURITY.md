# Security policy

## Scope

This repository is a reference firmware and hardware design for a TOTP hardware
token. Before reporting, please read [`docs/SECURITY.md`](docs/SECURITY.md) — some
properties people expect are **out of scope by design**, most importantly:

- **TOTP is not phishing-resistant.** A phished code can be replayed to the real
  site within its validity window. This is a property of TOTP itself, not a bug in
  this implementation. For phishing resistance, use FIDO2 / WebAuthn.
- **The protocol device key is fixed and public.** Device authentication in the
  Token2 wire protocol proves knowledge of a documented constant, not a per-device
  secret. This is inherited from the protocol, not a defect here.

In-scope issues include: deviations from the specified protocol behaviour,
cryptographic implementation bugs (SM4, HMAC, TOTP), memory-safety issues in the
firmware, and cases where the firmware could leak the seed.

## Time re-sync and the seed (and the C302i exception)

By default this firmware **clears the seed whenever the clock or configuration is
written** — see [`cmd_config` in `firmware/src/token2.c`](../firmware/src/token2.c#L189),
where the seed is wiped when the [`restricted_sync`](../firmware/include/token2.h#L34)
flag is set (its default state, initialized [here](../firmware/src/token2.c#L50)).
This is done for security purposes and in line with common security sense: because TOTP
codes are a pure function of `HMAC(seed, time)`, a device that let its clock be moved
while keeping the seed would let anyone who can reach it over NFC wind the clock back
to a moment when a valid code was previously observed and have the token regenerate
that same code — a straightforward replay attack. Wiping the seed on any time change
closes that window: after a clock change the token holds no secret and must be
re-provisioned, so there is nothing to replay.

Some production models — notably the **C302i** (and a couple of other models) —
deliberately relax this. On those units the
[`restricted_sync`](../firmware/include/token2.h#L34) flag is cleared so that the
clock can be re-synced **without** wiping the seed (the clear happens in
[`cmd_config`](../firmware/src/token2.c#L189)), letting a token whose crystal has
drifted be corrected in the field instead of re-issued. This is a conscious
trade-off, made with eyes open: it re-opens the replay window described above for
the interval an attacker can influence the clock, in exchange for operational
convenience on specific deployments. **This relaxed time-sync behavior is a
property of those specific models and is not enabled by the open-source default
here** — the default build wipes the seed on every time change. If you build a fork
that clears `restricted_sync`, understand that you are accepting the replay
exposure.

We treat this as an openly documented trade-off, not a hidden weakness: the behavior
is described on the affected products' pages, and the residual replay risk is bounded
by the validation server, which per RFC 6238 §5.2 **MUST NOT** accept a
previously-used OTP within a time step. RFC 6238 §6 *RECOMMENDS* that servers detect
and compensate for a token's clock drift (recording the token's time-step offset and
adjusting future validations). **We consider that recommendation mandatory** — a
well-behaved server should handle natural crystal drift itself. These models exist
precisely to serve back-ends that don't: where a server won't compensate for drift,
correcting the token's clock in the field is the practical alternative to re-issuing
hardware.

We consider this an acceptable and openly documented trade-off rather than a hidden
weakness: the behavior is described on the affected products' pages, and the residual
replay risk is bounded by the validation server, which per RFC 6238 §5.2 **MUST NOT**
accept a previously-used OTP within a time step. These models exist specifically to
serve authentication back-ends that do **not** implement the drift detection and
compensation that RFC 6238 §6 *RECOMMENDS* (server-side resync of a token's recorded
clock offset). Where a server refuses to compensate for a token's natural crystal
drift, correcting the token's clock in the field is the practical alternative to
re-issuing hardware — so these models were built to address that server-side gap.

## Reporting

Please report suspected security issues [`privately`](https://www.token2.swiss/site/page/informing-us-about-security-issues) first, rather than opening a
public issue:

- Use GitHub's private vulnerability reporting ("Report a vulnerability" under the
  Security tab), or
- Contact the maintainers at the address listed in the repository profile.

Include a clear description, affected files or commits, and a reproduction if
possible. We'll acknowledge receipt and work with you on a coordinated disclosure
timeline.

## Please do not

- Open public issues containing exploit details before a fix is available.
- Test against devices you don't own.
