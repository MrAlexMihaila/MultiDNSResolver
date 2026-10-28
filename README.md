# MultiDNSResolver
Simple Multi-Threaded DNS Resolver made in C, passing hostnames into a shared array from requesters to then be resolved by resolver threads for processing of hostnames into actual IP addresses

# Usage
'multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]'

# Input Files
Input files must simply list out one hostname for each line. For example...

```
facebook.com
youtube.com
yahoo.com
live.com
wikipedia.org
```

# Results
Requester log simply outputs the hostnames that were sent to be resolved.

Resolver log outputs hostnames and their IP address. If no IP address is found, then NOT_RESOLVED is put instead
