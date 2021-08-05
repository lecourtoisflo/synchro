#pragma once

namespace synchro
{
template<class... Ts>
struct Required
{
};

template<class... Ts>
struct Optional
{
};

template<class... Ts>
struct List
{
};

template<template<class... Ts> class Required, template<class... Us> class Optional,
         template<class... Ls> class List>
class SynchronizedData
{
};

} // namespace synchro
