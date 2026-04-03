#!/bin/bash
# FiXPro - GitHub Repository Setup Script
# Run this script to initialize Git and push to GitHub

set -e

echo "========================================="
echo "FiXPro - GitHub Repository Setup"
echo "========================================="
echo ""

# Check if git is installed
if ! command -v git &> /dev/null; then
    echo "❌ Git is not installed. Please install git first."
    exit 1
fi

# Check if gh is installed
if ! command -v gh &> /dev/null; then
    echo "⚠️  GitHub CLI (gh) is not installed."
    echo "   Install from: https://cli.github.com/"
    echo "   Or skip GitHub repo creation and just initialize local git."
    echo ""
    read -p "Continue with local git only? (y/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
    CREATE_REPO=false
else
    CREATE_REPO=true
fi

# Navigate to project directory
cd "$(dirname "$0")"

# Initialize git if not already initialized
if [ ! -d .git ]; then
    echo "📦 Initializing Git repository..."
    git init
    git branch -M main
else
    echo "✓ Git repository already initialized"
fi

# Add remote if specified
if [ "$CREATE_REPO" = true ]; then
    echo ""
    echo "🌐 Creating GitHub repository..."
    
    # Check if remote already exists
    if git remote -v | grep -q origin; then
        echo "✓ Remote 'origin' already exists"
    else
        echo "Creating remote repository..."
        gh repo create fxpro-project/fixpro \
            --public \
            --description "FiXPro - Flash iX Pro. Universal hardware programmer built on Raspberry Pi RP2040" \
            --homepage "https://fx.in.ua" \
            --source . \
            --push \
            --remote origin
        
        echo "✓ Repository created and pushed!"
    fi
fi

# Add all files
echo ""
echo "📝 Adding files to Git..."
git add -A

# Check for any staged files
if git diff --cached --quiet; then
    echo "✓ No changes to commit"
else
    # Create initial commit
    echo ""
    read -p "Enter commit message (default: 'Initial commit: FiXPro v1.0.0'): " COMMIT_MSG
    COMMIT_MSG=${COMMIT_MSG:-"Initial commit: FiXPro v1.0.0"}
    
    echo "💾 Creating initial commit..."
    git commit -m "$COMMIT_MSG"
    
    echo "✓ Initial commit created!"
    
    # Push to remote
    if git remote -v | grep -q origin; then
        echo ""
        echo "🚀 Pushing to GitHub..."
        git push -u origin main
        echo "✓ Pushed to GitHub!"
    fi
fi

echo ""
echo "========================================="
echo "✅ Setup complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "1. Visit your repository at: https://github.com/fxpro-project/fixpro"
echo "2. Enable GitHub Pages in repository Settings > Pages"
echo "3. Set Source to 'Deploy from a branch' and select 'gh-pages'"
echo "4. Your web interface will be available at: https://fxpro-project.github.io/fixpro"
echo "5. Create a release tag to trigger firmware build:"
echo "   git tag v1.0.0"
echo "   git push origin v1.0.0"
echo ""
echo "GitHub Actions will:"
echo "  - Build firmware on every push"
echo "  - Create releases on every tag"
echo "  - Deploy web interface to GitHub Pages"
echo ""
