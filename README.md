# ngx_http_access_control_module

# Name

A custom Nginx module for advanced access control based on variables.

# Table of Content

- [ngx\_http\_access\_control\_module](#ngx_http_access_control_module)
- [Name](#name)
- [Table of Content](#table-of-content)
- [Status](#status)
- [Synopsis](#synopsis)
- [Installation](#installation)
- [Directives](#directives)
  - [access](#access)
  - [access\_rules\_inherit](#access_rules_inherit)
  - [access\_deny\_status](#access_deny_status)
- [Author](#author)
- [License](#license)

# Status

This Nginx module is currently considered experimental. Issues and PRs are welcome if you encounter any problems.

# Synopsis

```nginx
server {
    listen 80;
    server_name example.com;

    # Allow access if $var2 is non-empty and not zero. The allowed request will no longer match the remaining access control rules.
    access allow $var1;

    # Deny access if $var1 is non-empty and not zero
    access deny $var2;

    location / {
        # Your other configurations
    }

    location /restricted {
        # Override deny status code
        access_deny_status 404;

        # Deny access if $var3 is non-empty and not zero
        access deny $var3;
    }
}
```

# Installation

To use theses modules, configure your nginx branch with `--add-module=/path/to/ngx_http_access_control_module`.

# Directives

## access

**Syntax:** *access [allow|deny] variable;*

**Default:** *-*

**Context:** *http, server, location*

The access directive defines an access control rule based on a variable. The variable is evaluated at runtime, and if it is non-empty and not zero, the rule is considered matched.

The `allow` parameter allows access if the condition is met. The allowed request will no longer match the remaining access control rules.
The `deny`  parameter denies access if the condition is met.

## access_rules_inherit

**Syntax:** *access_rules_inherit on | off | before | after;*

**Default:** *access_rules_inherit on;*

**Context:** *http, server, location*

Allows altering inheritance rules for the values specified in the `access` directive. By default, the standard inheritance model is used.

The `before` parameter specifies that the rules inherited from the previous configuration level will be applied before the rules specified in the current location block.

the `after` parameter specifies that the rules inherited from the previous configuration level will be applied after the rules specified in the current location block.

The `off` parameter cancels inheritance of the values from the previous configuration level.

## access_deny_status

**Syntax:** *access_deny_status code;*

**Default:** *access_deny_status 403;*

**Context:** *http, server, location*

Sets the HTTP status code to return in response when access is denied by a deny rule.

# Author

Hanada im@hanada.info

# License

This Nginx module is licensed under [BSD 2-Clause License](LICENSE).
