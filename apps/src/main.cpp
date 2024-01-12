//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "apps/nodesetexporter/Application.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char const* argv[])
{
    try
    {
        apps::nodesetexporter::Application app(std::span<const char*>(argv, argc));
        return app.Run();
    }
    catch (...)
    {
        std::cout << "An unexpected exception has occurred in the program." << std::endl;
        return EXIT_FAILURE;
    }
}