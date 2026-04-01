"""Build the vibration C extension with setuptools."""

from setuptools import Extension, setup

setup(
    name="vibration",
    version="1.0.0",
    description="Vibration / signal statistics implemented in C",
    ext_modules=[
        Extension(
            "vibration",
            sources=["vibrationmodule.c"],
            extra_compile_args=["-Wall", "-O2", "-std=c99"],
        )
    ],
    zip_safe=False,
)
