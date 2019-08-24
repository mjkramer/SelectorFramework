#!/bin/bash
find . -name '*ACLiC*' -o -name '*.d' -o -name '*.so' | xargs rm 2>/dev/null
