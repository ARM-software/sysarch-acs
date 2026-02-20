#!/bin/bash
## @file
#  Verify vendor INF files include all upstream tests
#
#  Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
#  SPDX-License-Identifier : Apache-2.0
#
#  Usage: ./verify_vendor_inf.sh [vendor_name]
#         ./verify_vendor_inf.sh nvidia
#         ./verify_vendor_inf.sh        # verifies all vendors
##

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APPS_DIR="$(dirname "$SCRIPT_DIR")"

# Extract test sources from an INF file (excluding vendor tests and app sources)
extract_test_sources() {
    local inf_file="$1"
    grep -E '^\s*(\.\./)*test_pool/' "$inf_file" | \
        grep -v 'vendor/' | \
        sed 's|.*test_pool/|test_pool/|' | \
        sed 's|\.c.*|.c|' | \
        sort -u
}

# Compare upstream and vendor INF
verify_inf_pair() {
    local upstream_inf="$1"
    local vendor_inf="$2"
    local upstream_name=$(basename "$upstream_inf" .inf)
    local vendor_name=$(basename "$vendor_inf" .inf)

    if [ ! -f "$upstream_inf" ]; then
        echo "ERROR: Upstream INF not found: $upstream_inf"
        return 1
    fi

    if [ ! -f "$vendor_inf" ]; then
        echo "ERROR: Vendor INF not found: $vendor_inf"
        return 1
    fi

    echo "Checking: $upstream_name -> $vendor_name"

    # Extract sources
    local upstream_sources=$(extract_test_sources "$upstream_inf")
    local vendor_sources=$(extract_test_sources "$vendor_inf")

    # Find missing sources
    local missing=""
    for src in $upstream_sources; do
        if ! echo "$vendor_sources" | grep -q "^${src}$"; then
            missing="$missing  - $src\n"
        fi
    done

    if [ -n "$missing" ]; then
        echo "  FAIL: Missing tests in $vendor_name:"
        echo -e "$missing"
        return 1
    else
        echo "  PASS: All upstream tests present"
        return 0
    fi
}

# Verify a vendor
verify_vendor() {
    local vendor="$1"
    local vendor_dir="$SCRIPT_DIR/$vendor"
    local errors=0
    local found_infs=0

    echo "========================================"
    echo "Verifying vendor: $vendor"
    echo "========================================"

    # Check if vendor directory exists
    if [ ! -d "$vendor_dir" ]; then
        echo "WARNING: Vendor directory not found: $vendor_dir"
        echo "  No vendor INFs to verify (this is OK if vendor tests not yet added)"
        return 0
    fi

    # Check each INF file in vendor directory
    # Look for pattern: <AcsType><Vendor>.inf (e.g., BsaNvidia.inf, XbsaAcpiNvidia.inf)
    for vendor_inf in "$vendor_dir"/*.inf; do
        [ -f "$vendor_inf" ] || continue
        
        local inf_basename=$(basename "$vendor_inf" .inf)
        
        # Skip if doesn't match vendor naming pattern
        # Extract the ACS type by removing vendor name suffix (case insensitive)
        local vendor_upper="${vendor^}"  # Capitalize first letter
        local acs_type=$(echo "$inf_basename" | sed "s/${vendor_upper}$//i")
        
        # Skip if no ACS type extracted (means it's not a vendor INF)
        [ -z "$acs_type" ] && continue
        [ "$acs_type" == "$inf_basename" ] && continue
        
        ((found_infs++))

        # Determine upstream INF name
        local upstream_inf="$APPS_DIR/${acs_type}.inf"

        if [ -f "$upstream_inf" ]; then
            verify_inf_pair "$upstream_inf" "$vendor_inf" || ((errors++))
        else
            echo "WARNING: No upstream INF found for $vendor_inf (expected: $upstream_inf)"
        fi
    done

    if [ $found_infs -eq 0 ]; then
        echo "  No vendor INF files found (this is OK if vendor tests not yet added)"
        return 0
    fi

    return $errors
}

# Main
main() {
    local total_errors=0
    local vendors_checked=0

    if [ -n "$1" ]; then
        # Verify specific vendor
        verify_vendor "$1"
        total_errors=$?
        vendors_checked=1
    else
        # Verify all vendors
        for vendor_dir in "$SCRIPT_DIR"/*/; do
            [ -d "$vendor_dir" ] || continue
            vendor=$(basename "$vendor_dir")
            [ "$vendor" = "." ] && continue
            [ "$vendor" = "template" ] && continue  # Skip template directory
            verify_vendor "$vendor"
            [ $? -ne 0 ] && ((total_errors++))
            ((vendors_checked++))
        done
    fi

    echo ""
    if [ $vendors_checked -eq 0 ]; then
        echo "No vendor directories found. Vendor infrastructure ready for use."
        exit 0
    elif [ $total_errors -eq 0 ]; then
        echo "All vendor INF files are in sync with upstream."
        exit 0
    else
        echo "ERROR: $total_errors vendor(s) have sync issues."
        echo "Please update vendor INF files to match upstream."
        exit 1
    fi
}

main "$@"
