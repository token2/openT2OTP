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
