"""
FiXPro CLI - Command Line Interface for FiXPro Universal Programmer

Usage:
    python -m fixpro [options] command [args]
    fixpro [options] command [args]
"""

from .main import FiXProDevice, __version__, main

__all__ = ["main", "FiXProDevice", "__version__"]
