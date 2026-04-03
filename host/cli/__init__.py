"""
FiXPro CLI - Command Line Interface for FiXPro Universal Programmer

Usage:
    python -m fixpro [options] command [args]
    fixpro [options] command [args]
"""

from .main import main, FiXProDevice, __version__

__all__ = ['main', 'FiXProDevice', '__version__']
