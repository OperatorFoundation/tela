#!/usr/bin/env fish

# Update all submodules to track their default branch (main or master)
function fix_submodules
    # Update submodule references (non-recursive)
    git submodule update --init
    
    # Get list of all submodules
    set submodules (git config --file .gitmodules --get-regexp path | awk '{ print $2 }')
    
    for submodule in $submodules
        echo "Processing submodule: $submodule"
        
        # Enter submodule directory
        cd $submodule
        
        # Fetch latest from origin
        git fetch origin
        
        # Determine default branch (try main first, then master)
        if git show-ref --verify --quiet refs/remotes/origin/main
            set default_branch main
        else if git show-ref --verify --quiet refs/remotes/origin/master
            set default_branch master
        else
            echo "  Warning: Could not find main or master branch"
            cd -
            continue
        end
        
        echo "  Default branch: $default_branch"
        
        # Checkout and track the default branch
        git checkout $default_branch
        git branch --set-upstream-to=origin/$default_branch $default_branch
        git pull
        
        # Return to root
        cd -
    end
    
    echo "Done! All submodules updated."
end

fix_submodules
